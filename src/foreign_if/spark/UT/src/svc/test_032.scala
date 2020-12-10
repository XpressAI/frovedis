package test.scala;

import com.nec.frovedis.Jexrpc.FrovedisServer
import com.nec.frovedis.exrpc.FrovedisLabeledPoint
import com.nec.frovedis.mllib.classification.{SVC, SVCModel}
import com.nec.frovedis.matrix.FrovedisRowmajorMatrix
import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.mllib.regression.LabeledPoint
import org.apache.spark.rdd.RDD
import org.apache.spark.mllib.linalg.{Vector, Vectors}
import org.apache.log4j.{Level, Logger}
import com.nec.frovedis.exrpc.FrovedisSparseData

object GenericTest {
  def main(args: Array[String]): Unit = {
    Logger.getLogger("org").setLevel(Level.ERROR)

    // -------- configurations --------
    val conf = new SparkConf().setAppName("SVRexample").setMaster("local[2]")
    val sc = new SparkContext(conf)
    sc.setLogLevel("ERROR")
    // initializing Frovedis server with "personalized command", if provided in command line
    if(args.length != 0) FrovedisServer.initialize(args(0))
    val data = Vector(
       Vectors.dense(-1,1),
       Vectors.dense(-2, -1),
       Vectors.dense(-3, -2),
       Vectors.dense(1,1),
       Vectors.dense(2, 1),
       Vectors.dense(3, 2)
    )

    val lbl = Vector(10, 30, 10, 20, 10, 20)
    val zip1 = lbl zip data
    var lpv = zip1.map( a => LabeledPoint(a._1, a._2) ) // vector of LabeledPoint
    var d_lp: RDD[LabeledPoint] = sc.parallelize(lpv)  // distributed LabeledPoint
    val f_lp = new FrovedisLabeledPoint(d_lp, false) // frovedis LabeledPoint

    val C = 1.0
    val kernelType = "linear"
    val degree = 3
    val gamma = 0.1
    val coef0 = 0.0
    val tol = 0.001
    val cacheSize = 128
    val maxIter = 100

    try {
        val model = SVC.train(f_lp, C, kernelType, degree, gamma, coef0, tol, cacheSize, maxIter)
    
        // --- prediction Vector---
        var test_data = sc.parallelize(data)
        val pred1 = model.predict(test_data).collect()
        // --- prediction RMM ---
        var frov_test_data = new FrovedisRowmajorMatrix(test_data)
        var pred2 = model.predict(frov_test_data).collect()
        println("Status : Failed") //Negative case
        model.release()
    }
    catch {
        case e: Exception => println("Status : Passed") //expected result
    }
    
    // -------- clean-up --------
    FrovedisServer.shut_down()
    sc.stop()
  }
}

