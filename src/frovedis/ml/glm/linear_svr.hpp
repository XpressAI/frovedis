#ifndef __LIN_SVR__
#define __LIN_SVR__

#include <frovedis/ml/metrics.hpp>
#include <frovedis/ml/model_selection/param.hpp>
#include <frovedis/ml/glm/svm_regression_with_sgd.hpp>

namespace frovedis {

template <class T>
struct linear_svm_regressor {
  linear_svm_regressor(int max_iter = 1000,
                       double alpha = 0.01,
                       const std::string& solver = "sgd",
                       double reg_param = 0.01,
                       const std::string& reg_type= "ZERO",
                       bool fit_intercept = false,
                       double mini_batch_fraction = 1.0,
                       double epsilon=0.0,
		       const std::string& loss_type= "EPS",
                       double convergence_tol = 0.001) {
    this->max_iter = max_iter;
    this->alpha = alpha;
    this->solver = solver;
    this->reg_param = reg_param;
    this->reg_type = reg_type;
    this->fit_intercept = fit_intercept;
    this->mbf = mini_batch_fraction;
    this->epsilon = epsilon;
    this->loss_type = loss_type;
    this->tol = convergence_tol;
    this->is_fitted = false;
    this->n_iter_ = 0;
  }
  linear_svm_regressor<T>& 
  set_max_iter(int max_iter) {
    std::string msg = "expected a positive max_iter; received: " + STR(max_iter) + "\n";
    require(max_iter > 0, msg);
    this->max_iter = max_iter;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_alpha(double alpha) {
    std::string msg = "expected a positive alpha (learning rate); received: " + STR(alpha) + "\n";
    require(alpha > 0, msg);
    this->alpha = alpha;
    return *this; 
  }
  linear_svm_regressor<T>& 
  set_solver(const std::string& solver) {
    std::string msg = "expected sgd; received: " + solver + "\n";
    require(solver == "sgd", msg);
    this->solver = solver;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_reg_param(double reg_param) {
    std::string msg = "expected a positive regularization parameter; received: " + STR(reg_param) + "\n";
    require(reg_param > 0, msg);
    this->reg_param = reg_param;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_reg_type(const std::string& reg_type) {
    std::string msg = "expected ZERO, L1 or L2; received: " + reg_type + "\n";
    require(reg_type == "ZERO" || reg_type == "L1" || reg_type == "L2", msg);
    this->reg_type = reg_type;
    return *this;  
  }
  linear_svm_regressor<T>&
  set_loss_type(const std::string& loss_type) {
    std::string msg = "expected EPS (epsilon-insensitive) or ";
    msg += "SQEPS (squared-epsilon-insensitive); received: " + loss_type + "\n";
    require(loss_type == "EPS" || loss_type == "SQEPS", msg);
    this->loss_type = loss_type;
    return *this;
  }
  linear_svm_regressor<T>& 
  set_intercept(bool is_intercept) {
    this->fit_intercept = is_intercept;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_mini_batch_fraction(double mbf) {
    std::string msg = "expected a positive mini-batch fraction; received: " + STR(mbf) + "\n";
    require(mbf > 0, msg);
    this->mbf = mbf;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_epsilon(int epsilon) {
    std::string msg = "expected a positive epsilon; received: " + STR(epsilon) + "\n";
    require(epsilon >= 0, msg);
    this->epsilon = epsilon;
    return *this;  
  }
  linear_svm_regressor<T>& 
  set_tol(double tol) {
    std::string msg = "expected a positive convergence tolerance; received: " + STR(tol) + "\n";
    require(tol > 0, msg);
    this->tol = tol;
    return *this;  
  }

  // frovedis::grid_sdearch_cv compatible setter
  linear_svm_regressor<T>& 
  set_params(std::vector<std::pair<std::string, param_t>>& data) {
    std::string msg = "";
    for (auto& e: data) {
      auto param = e.first;
      auto val = e.second;
      if(param == "max_iter") {              
        set_max_iter(val.get<int>()); 
        msg += "max_iter: " + val.tt + "; ";
      }
      else if(param == "alpha") { 
        set_alpha(val.get<double>());
        msg += "alpha: " + val.tt + "; ";
      }
      else if(param == "mini_batch_fraction") { 
        set_mini_batch_fraction(val.get<double>());
        msg += "mini batch fraction: " + val.tt + "; ";
      }
      else if(param == "epsilon") {              
        set_epsilon(val.get<double>()); 
        msg += "epsilon: " + val.tt + "; ";
      }
      else if(param == "tol") { 
        set_tol(val.get<double>());
        msg += "tol: " + val.tt + "; ";
      }
      else if(param == "reg_param") { 
        set_reg_param(val.get<double>());
        msg += "reg param: " + val.tt + "; ";
      }
      else if(param == "reg_type") {
        set_reg_type(val.get<std::string>());
        msg += "reg type: " + val.tt + "; ";
      }
      else if(param == "loss_type") {
	set_loss_type(val.get<std::string>());
	msg += "loss_type: " + val.tt + "; ";
      }
      else if(param == "solver") {
        set_solver(val.get<std::string>());
        msg += "solver: " + val.tt + "; ";
      }
      else if(param == "fit_intercept") { 
        set_intercept(val.get<bool>());
        msg += "fit intercept: " + val.tt + "; ";
      }
      else REPORT_ERROR(USER_ERROR, "[linear_svm_regressor] Unknown parameter: '" 
                                     + param + "' is encountered!\n");
    }
    RLOG(DEBUG) << msg << "\n";
    return *this;
  }

  RegType get_regularizer() {
    RegType regularizer = ZERO;
    if (reg_type == "ZERO") regularizer = ZERO;
    else if (reg_type == "L1") regularizer = L1;
    else if (reg_type == "L2") regularizer = L2;
    else REPORT_ERROR(USER_ERROR, "[linear_svm_regressor] Unknown regularizer '" 
                                   + reg_type + "' is encountered!\n");
    return regularizer;
  }

  SVRLossType get_loss() {
    SVRLossType loss = EPS;
    if (loss_type == "EPS") loss = EPS;
    else if (loss_type == "SQEPS") loss = SQEPS;
    else REPORT_ERROR(USER_ERROR, "[linear_svm_regressor] Unknown loss '"
                                   + loss_type + "' is encountered!\n");
    return loss;
}

  // MATRIX: can accept both rowmajor and colmajor matrices as for dense data; 
  //         and crs matrix as for sparse data
  template <class MATRIX>
  linear_svm_regressor& 
  fit(MATRIX& mat, dvector<T>& label
      const std::vector<T> &sample_weight = std::vector<T>()) {
    size_t n_iter;
    if (solver == "sgd") {
      this->model = svm_regression_with_sgd::train(
                      mat, label, sample_weight, n_iter, max_iter, alpha, mbf,
                      reg_param, get_regularizer(), fit_intercept, 
                      tol, epsilon, get_loss());
    }
    else REPORT_ERROR(USER_ERROR, "Unknown solver is encountered!\n");
    this->is_fitted = true;
    this->n_iter_ = n_iter;
    return *this;
  }

  template <class MATRIX>
  std::vector<T> 
  predict(MATRIX& mat) {
    if(!is_fitted) REPORT_ERROR(USER_ERROR, 
                   "[linear_svm_regressor] predict is called before fit\n");
    return mat.data.map(parallel_predict<typename MATRIX::local_mat_type,
                        linear_regression_model<T>, T>,
                        broadcast(model))
                   .template moveto_dvector<T>()
                   .gather();
  }

  template <class MATRIX>
  float score(MATRIX& mat, dvector<T>& label) {
    if(!is_fitted) REPORT_ERROR(USER_ERROR, 
                   "[linear_svm_regressor] score is called before fit\n");
    auto pred_label = predict(mat);
    return r2_score(pred_label, label.gather());
  }

  int max_iter;
  double alpha, mbf, tol, reg_param, epsilon;
  std::string reg_type, solver, loss_type;
  bool fit_intercept;
  linear_regression_model<T> model;
  bool is_fitted;
  size_t n_iter_;
  SERIALIZE(max_iter, epsilon, alpha, mbf, tol, 
            reg_param, reg_type, loss_type, solver,
            fit_intercept, model, is_fitted, n_iter_); 
};

}
#endif
