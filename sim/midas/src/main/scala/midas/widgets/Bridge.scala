// See LICENSE for license details.

package midas
package widgets

import midas.core.{TargetChannelIO}

import freechips.rocketchip.config.{Parameters, Field}

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule, Direction, ChiselAnnotation, annotate}
import firrtl.annotations.{ReferenceTarget, ModuleTarget, JsonProtocol, HasSerializationHints}

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

    // ------------------------------------------------------

    tokenHashers2()

    super.genCRFile()
  }

  def tokenHashers2() = {


    println("Entering tokenHashers2()")


    val thelist = hPort.getOutputChannelPorts()
    

    thelist.map({ case (name,ch) =>

      println(s"OUTPUT Channel ${name}")
      // PipeBridgeChannel(name, meta.clockRT, meta.fieldRTs, Seq(), 0)

      hashRecord += name


      for (x <- hashRecord) {
        println(f"Found ${x}")
      }

      // dontTouch(ch.ready)
      
      val shouldHash = ch.fire
      dontTouch(shouldHash)

      val ZZZoutputChannelHash = XORHash32(ch.bits, shouldHash)
      // val ZZZoutputChannelHash = XORHash32(ch.bits, ch.valid)
      chisel3.assert(ZZZoutputChannelHash===RegNext(ZZZoutputChannelHash))

      val readHash = genROReg(ZZZoutputChannelHash, s"readHash${name}")

      val (counter, counterWrap) = Counter.apply(shouldHash, 256)

      // val shouldHash2 = counter < 3

      // val fifoDepth = 1024
      val fifoDepth = 128

      val q = Module(new BRAMQueue(fifoDepth)(UInt(32.W)))
      q.io.enq.valid := shouldHash
      // q.io.enq.valid := shouldHash2
      q.io.enq.bits := counter
      // enq.ready := q.io.enq.ready
      // q.io.deq
      
      // q.io.deq.bits
      // val readQueue = genROReg(q.io.deq.bits, s"readQueue${name}")
      val readRdy   = genROReg(q.io.enq.ready, s"readReady${name}")
      // val writeValid   = genWOReg(q.io.deq.valid, s"writeValid${name}")
      // val readRdy2   = genWOReg(q.io.deq.ready, s"reeadDeqReady${name}")



      // val rDataQ = Module(new MultiWidthFifo(hWidth, cWidth, 2))
      attachDecoupledSource(q.io.deq, s"attachReadReady${name}")
      // memNasti.r.ready := rDataQ.io.in.ready
      // rDataQ.io.in.valid := memNasti.r.valid
      // rDataQ.io.in.bits := memNasti.r.bits.data


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
