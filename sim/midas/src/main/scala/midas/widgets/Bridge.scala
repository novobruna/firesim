// See LICENSE for license details.

package midas
package widgets

import midas.core.{TargetChannelIO}

import freechips.rocketchip.config.{Parameters, Field}

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule, Direction, ChiselAnnotation, annotate}
import firrtl.annotations.{ReferenceTarget, ModuleTarget, JsonProtocol, HasSerializationHints}
import freechips.rocketchip.util.WideCounter

import scala.reflect.runtime.{universe => ru}

import scala.collection.mutable

/* Bridge
 *
 * Bridges are widgets that operate directly on token streams moving to and
 * from the transformed-RTL model.
 *
 */

// Set in FPGA Top before the BridgeModule is generated
case object TargetClockInfo extends Field[Option[RationalClock]]

abstract class BridgeModule[HostPortType <: Record with HasChannels]()(implicit p: Parameters) extends Widget()(p) {
  def module: BridgeModuleImp[HostPortType]
}

class TokenHasherControlBundle extends Bundle {
  val triggerDelay = Input(UInt(64.W))
  val triggerPeriod = Input(UInt(64.W))
}

case class TokenHasherMeta(
  bridgeName:    String,  // the name of bridge
  name:      String,      // the name of the channel
  output:    Boolean,     // true if this is an output port
  queueHead: Int,         // local MMIO address of the queue head
  queueOccupancy: Int,
  tokenCount0:    Int,
  tokenCount1:    Int
)

abstract class BridgeModuleImp[HostPortType <: Record with HasChannels]
    (wrapper: BridgeModule[_ <: HostPortType])
    (implicit p: Parameters) extends WidgetImp(wrapper) {
  def hPort: HostPortType
  def clockDomainInfo: RationalClock = p(TargetClockInfo).get
  def emitClockDomainInfo(headerWidgetName: String, sb: StringBuilder): Unit = {
    import CppGenerationUtils._
    val RationalClock(domainName, mul, div) = clockDomainInfo
    sb.append(genStatic(s"${headerWidgetName}_clock_domain_name", CStrLit(domainName)))
    sb.append(genConstStatic(s"${headerWidgetName}_clock_multiplier", UInt32(mul)))
    sb.append(genConstStatic(s"${headerWidgetName}_clock_divisor", UInt32(div)))
  }

  val tokenHasherControlIO = IO(new TokenHasherControlBundle())

  // only use for meta data
  val hashRecord = mutable.ArrayBuffer[String]()

  override def genCRFile(): MCRFile = {
    // ------------------------------------------------------
    // arrayBuffer (defined bridge module imp)
    // create mutable structure here to capture data about MIMO related to tokenHahers fifo / controls
    // channelName
    // direction
    // mmio address
    //   dequeue (128 deep)
    //   head

    // bramqueue (naturally 36k bits) (means queue should be 1024)

    // add another IO to bridge modile imp "hasher config io", drive from fpga top
    //   delay period
    //   frequency

    // widget called simulation master, "config io" in there, and then connect to all other bridges
    
    
    // add ("hasher config io")registers to:
    // /home/centos/firesim/sim/midas/src/main/scala/midas/widgets/Master.scala

    // option:
    // delay and frequency per clock domain
    //   emitClockDomainInfo

    // add a new field / case class to parameters
    // (name of field should end in "key")
    // type of the key is Field[Option[TokenHasherParams]]
    //   inside should be fifo depths
    //   counter widths?
    //   hash vs counter operation

    // one test with at least multiple bridges
    // test both directions

    // a peek poke bridge in loopback will test token hashers in both directions


    // ------------------------------------------------------

    tokenHashers2()

    super.genCRFile()
  }

  def tokenHashers2() = {


    println("Entering tokenHashers2()")


    val thelist = hPort.getOutputChannelPorts()
    

    thelist.map({ case (name,ch) =>
      

      val USE_COUNTER_FOR_HASH: Boolean = true

      println(s"OUTPUT Channel ${name}")
      // PipeBridgeChannel(name, meta.clockRT, meta.fieldRTs, Seq(), 0)

      hashRecord += name


      for (x <- hashRecord) {
        println(f"Found ${x}")
      }


      // how many tokens have we seen
      val tokenCount = WideCounter(width = 64, inhibit = !ch.fire).value
      
      val triggerDelay = tokenHasherControlIO.triggerDelay
      val triggerFrequency = tokenHasherControlIO.triggerPeriod


      val delayMatchMulti = triggerDelay === tokenCount
      val delayMatch = delayMatchMulti && !RegNext(delayMatchMulti)
      val triggerStart = RegInit(false.B)
      

      // true when the period counter should reset
      val periodCountReset = Wire(Bool())
      
      // counter that advances each time the channel fires
      // this is used to determine which hashes we will use
      val (periodCount: UInt, periodCountOverflow) = Counter.apply(Range(0, 2147483647, 1), enable=ch.fire, reset=periodCountReset)
      // val (periodCount: UInt, periodCountOverflow) = Counter.apply(Range(0, 2147483647, 1), enable=true.B, reset=periodCountReset)

      val periodMatch = periodCount === triggerFrequency(30, 0)

      // lazy val periodCountReset: Bool = (delayMatch | periodMatch)
      when(delayMatch | periodMatch) {
        periodCountReset := true.B & ch.fire
      }.otherwise{
        periodCountReset := false.B
      }

      val periodOK = periodCountReset


      // only set triggerStart when the delay counter matches
      // this will stay high
      when(delayMatch) {
        triggerStart := true.B
      }



      val chFire = ch.fire
      
      
      // val shouldHash = ch.fire
      val shouldHash = periodOK & triggerStart
      // val shouldHash = RegNext(shouldHashUndelay)
      

      val ZZZoutputChannelHash = XORHash32(ch.bits, ch.fire)
      // val ZZZoutputChannelHash = XORHash32(ch.bits, ch.valid)
      // chisel3.assert(ZZZoutputChannelHash===RegNext(ZZZoutputChannelHash))

      // val readHash = genROReg(ZZZoutputChannelHash, s"readHash${name}")



      // fake hash to debug 
      val fakeHash = WideCounter(width = 32, inhibit = !ch.fire).value

      val useHash = if(USE_COUNTER_FOR_HASH) fakeHash else ZZZoutputChannelHash


      // val fifoDepth = 1024
      val fifoDepth = 128

      // 36K bits (32K usable bits)
      val q = Module(new BRAMQueue(fifoDepth)(UInt(32.W)))
      q.io.enq.valid := shouldHash
      q.io.enq.bits := useHash

      // not needed
      // val readRdy   = genROReg(q.io.enq.ready, s"readReady${name}")




      // val rDataQ = Module(new MultiWidthFifo(hWidth, cWidth, 2))
      attachDecoupledSource(q.io.deq, s"queueHead_${name}")
    

      val genOccupancy = genROReg(q.io.count, s"readQueueOccupancy_${name}")

      val counterLow    = genROReg(tokenCount(31,0), s"counterLow_${name}")
      val counterHigh   = genROReg(tokenCount(63,32), s"counterHigh_${name}")


      dontTouch(ZZZoutputChannelHash)
      dontTouch(triggerDelay)
      dontTouch(triggerFrequency)
      dontTouch(delayMatch)
      dontTouch(triggerStart)
      dontTouch(periodCountReset)
      dontTouch(periodMatch)
      dontTouch(chFire)
      dontTouch(shouldHash)
      dontTouch(fakeHash)
      dontTouch(periodOK)

      // q.io.deq.valid 
      // when(counter > 3.U) {
      //   q.io.deq.valid = true.B
      // }

    })

    Unit
  }

  // tokenHashers2()
  // hPort.tokenHashers()
  // lazy val setupTokenHashers = hPort.tokenHashers()
  // println("BridgeModuleImp")
  // println(hPort)
  // hPort.tokenHashers()


}

trait Bridge[HPType <: Record with HasChannels, WidgetType <: BridgeModule[HPType]] {
  self: BaseModule =>
  def constructorArg: Option[_ <: AnyRef]
  def bridgeIO: HPType
  // def bridgeIO: HPType with HasChannels

  def generateAnnotations(): Unit = {

    // Adapted from https://medium.com/@giposse/scala-reflection-d835832ed13a
    val mirror = ru.runtimeMirror(getClass.getClassLoader)
    val classType = mirror.classSymbol(getClass)
    // The base class here is Bridge, but it has not yet been parameterized.
    val baseClassType = ru.typeOf[Bridge[_,_]].typeSymbol.asClass
    // Now this will be the type-parameterized form of Bridge
    val baseType = ru.internal.thisType(classType).baseType(baseClassType)
    val widgetClassSymbol = baseType.typeArgs(1).typeSymbol.asClass

    // Generate the bridge annotation
    annotate(new ChiselAnnotation { def toFirrtl = {
        BridgeAnnotation(
          self.toNamed.toTarget,
          bridgeIO.bridgeChannels,
          widgetClass = widgetClassSymbol.fullName,
          widgetConstructorKey = constructorArg)
      }
    })
  }
}

trait HasChannels {
  /**
    * Returns a list of channel descriptors.
    */
  def bridgeChannels(): Seq[BridgeChannel]

  def getOutputChannelPorts(): Seq[(String,DecoupledIO[Data])]
  def getInputChannelPorts(): Seq[(String,DecoupledIO[Data])]

  // copy bridgeChannels
  // def tokenHashers(): Unit=Nil
  // def tokenHashers(): {}

  // Called in FPGATop to connect the instantiated bridge to channel ports on the wrapper
  private[midas] def connectChannels2Port(bridgeAnno: BridgeIOAnnotation, channels: TargetChannelIO): Unit
}
