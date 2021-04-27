import sys
import os.path
import pickle
import numpy as np
import numbers
from ..model_util import *
from ...base import *
from ...exrpc.server import FrovedisServer
from ...exrpc import rpclib
from ...matrix.ml_data import FrovedisFeatureData
from ...matrix.dense import FrovedisRowmajorMatrix
from ...matrix.dtype import TypeUtil


class GaussianMixture(BaseEstimator):
    """
    A python wrapper of Frovedis kmeans
    """
    def __init__(self, n_components=1, covariance_type='full', tol=1e-3,
                 reg_covar=1e-6, max_iter=100, n_init=1, init_params='kmeans',
                 weights_init=None, means_init=None, precisions_init=None,
                 random_state=None, warm_start=False,
                 verbose=0, verbose_interval=10):
                 
        self.n_components = n_components
        self.cov_type = covariance_type
        self.tol = tol
        self.max_iter = max_iter 
        self.n_init = n_init
        self.init_params = init_params
        self.random_state = random_state
        self.verbose = verbose        
        self.__mid = None
        self.__mdtype = None
        self.__mkind = M_KIND.GMM
        self.n_features = None
        self._weights = None
        self._means = None
        self._covariances = None
        self._converged = None
        self.movable = None        

    def check_input(self, X, F):
        inp_data = FrovedisFeatureData(X, \
                     caller = "[" + self.__class__.__name__ + "] "+ F +": ",\
                     dense_kind='rowmajor', densify=True)
        X = inp_data.get()        
        dtype = inp_data.get_dtype()
        itype = inp_data.get_itype()
        dense = inp_data.is_dense()
        nsamples = inp_data.numRows()
        nfeatures = inp_data.numCols()        
        movable = inp_data.is_movable()
        return X, dtype, itype, dense, nsamples, nfeatures, movable    

    def validate(self):
        """validates hyper parameters"""
        if self.n_init is None:
            self.n_init = 1
        if self.n_init < 1:
            raise ValueError("fit: n_init must be a positive integer!")
        if isinstance(self.random_state, numbers.Number):
            if sys.version_info[0] < 3:
                self.seed = long(self.random_state)
            else:
                self.seed = int(self.random_state)
        else:
            self.seed = 0            
        if(self.init_params not in ['kmeans', 'random']):
            raise ValueError("init_params: GMM doesn't support the "\
                              + "given init type!")
                              
        if(self.cov_type not in ['full']):
            raise ValueError("covariance_type: Frovedis doesn't support the "\
                              + "given covariance type!")            
 
    def fit(self, X, y=None):
        self.release()
        self.validate()    
        X, dtype, itype, dense, nsamples, \
        nfeatures, movable = self.check_input(X, "fit")
        self.n_features = nfeatures
        self.movable = movable
        self.__mdtype = dtype
        self.__mid = ModelID.get()
        (host, port) = FrovedisServer.getServerInstance()
        #Train and save the model
        niter = rpclib.gmm_train(host, port, X.get(), self.n_components,
                                 self.cov_type.encode('ascii'), 
                                 self.tol, self.max_iter, self.n_init,
                                 self.init_params.encode('ascii'), 
                                 self.seed, self.verbose, 
                                 self.__mid, dtype, itype, dense)
        
        self._weights = None
        self._means = None
        self._covariances = None
        self._converged = None
        self.n_iter_ = niter
        self._lower_bound = None
        excpt = rpclib.check_server_exception()
        if excpt["status"]:
            raise RuntimeError(excpt["info"])
        return self 

    def fit_predict(self, X, y=None):
        self.fit(X)
        labels = self.predict(X)        
        return labels
        
    def predict(self, X):
        """Predict the labels each sample in X belong to."""
        if self.__mid is None:
            raise ValueError( \
            "predict: is called before calling fit, or the model is released.")
        X, dtype, itype, dense, \
        nsamples, nfeatures, movable = self.check_input(X, "predict")
        if self.n_features != nfeatures:
            raise ValueError( \
            "predict: given features do not match with current model")        
        if dtype != self.__mdtype:
            raise TypeError( \
            "predict: datatype of X is different than model dtype!")
        (host, port) = FrovedisServer.getServerInstance()
        ret = np.zeros(nsamples, dtype=np.int64)
        rpclib.gmm_predict(host, port, self.__mid,
                           self.__mdtype, X.get(),
                           ret, nsamples)
        excpt = rpclib.check_server_exception()
        if excpt["status"]:
            raise RuntimeError(excpt["info"])
        return ret        
        
    def predict_proba(self, X):
        """Predict the probabilty of each component given X."""
        if self.__mid is None:
            raise ValueError( \
            "predict: is called before calling fit, or the model is released.")
        X, dtype, itype, dense, \
        nsamples, nfeatures, movable = self.check_input(X, "predict")
        if self.n_features != nfeatures:
            raise ValueError( \
            "predict: given features do not match with current model")        
        if dtype != self.__mdtype:
            raise TypeError( \
            "predict: datatype of X is different than model dtype!")
        (host, port) = FrovedisServer.getServerInstance()
        prob_vector = rpclib.gmm_predict_proba(host, port, self.__mid,
                                               self.__mdtype, X.get())
        shape = (nsamples, self.n_components)
        prob = np.asarray(prob_vector, dtype=np.float64).reshape(shape)
        excpt = rpclib.check_server_exception()
        if excpt["status"]:
            raise RuntimeError(excpt["info"])
        return prob
        
    def sample(self, n_samples=1):
        pass
        
    def score(self, X, y=None):
        pass

    def score_samples(self, X):
        pass    
             
    @property
    def weights_(self):
        """
        NAME: get_weights
        """
        if self.__mid is None:
            raise ValueError("weights_ is called before fit/load")

        if self._weights is None:
            (host, port) = FrovedisServer.getServerInstance()
            weights_vector = rpclib.get_gmm_weights(host, port, self.__mid, \
                                                  self.__mdtype)
            excpt = rpclib.check_server_exception()
            if excpt["status"]:
                raise RuntimeError(excpt["info"])
  
            self._weights = np.asarray(weights_vector, dtype=np.float64)
            
        return self._weights

    @weights_.setter
    def weights_(self, val):
        #Setter method for weights_
        raise AttributeError(\
        "attribute 'weights_' of Gaussian Mixture is not writable")

    @property
    def covariances_(self):
        """
        NAME: get_covariances
        """
        if self.__mid is None:
            raise ValueError("covariances_ is called before fit/load")

        if self._covariances is None:
            (host, port) = FrovedisServer.getServerInstance()
            covariance_vector = rpclib.get_gmm_covariances(host, port, self.__mid, \
                                                  self.__mdtype)
            excpt = rpclib.check_server_exception()
            if excpt["status"]:
                raise RuntimeError(excpt["info"])
            shape = (self.n_components, self.n_features, self.n_features)            
            self._covariances = np.asarray(covariance_vector, dtype = np.float64)\
                                  .reshape(shape)
            
        return self._covariances

    @covariances_.setter
    def covariances_(self, val):
        #Setter method for covariances_
        raise AttributeError(\
        "attribute 'covariances_' of Gaussian Mixture is not writable")

    @property
    def means_(self):
        """
        NAME: get_means
        """
        if self.__mid is None:
            raise ValueError("means_ is called before fit/load")

        if self._means is None:
            (host, port) = FrovedisServer.getServerInstance()
            means_vector = rpclib.get_gmm_means(host, port, self.__mid, \
                                                  self.__mdtype)
            excpt = rpclib.check_server_exception()
            if excpt["status"]:
                raise RuntimeError(excpt["info"])
            
            shape = (self.n_components, self.n_features)            
            self._means = np.asarray(means_vector, dtype = np.float64)\
                                  .reshape(shape)
            
        return self._means

    @means_.setter
    def means_(self, val):
        #Setter method for covariances_
        raise AttributeError(\
        "attribute 'means_' of Gaussian Mixture is not writable")        

    @property
    def converged_(self):
        """
        NAME: get_converged
        """
        if self.__mid is None:
            raise ValueError("converged_ is called before fit/load")

        if self._converged is None:
            self._converged = self.n_iter_ < self.max_iter
            
        return self._converged

    @converged_.setter
    def converged_(self, val):
        #Setter method for converged_
        raise AttributeError(\
        "attribute 'converged_' of Gaussian Mixture is not writable")
        

    @property
    def lower_bound_(self):
        """
        NAME: get_lower_bound
        """
        if self.__mid is None:
            raise ValueError("lower_bound_ is called before fit/load")
            
        if self._lower_bound is None:
            (host, port) = FrovedisServer.getServerInstance()
            lb = rpclib.get_gmm_lower_bound(host, port, self.__mid, \
                                                self.__mdtype)
            excpt = rpclib.check_server_exception()
            if excpt["status"]:
                raise RuntimeError(excpt["info"])
  
            self._lower_bound = lb
            
        return self._lower_bound

    @lower_bound_.setter
    def lower_bound_(self, val):
        #Setter method for lower_bound_
        raise AttributeError(\
        "attribute 'lower_bound_' of Gaussian Mixture is not writable")

    def load(self, fname, dtype=None):
        """
        NAME: load
        """
        if isinstance(fname, str) == False:
            raise TypeError("Expected: String, Got: " + str(type(fname)))
        if not os.path.exists(fname):
            raise ValueError(\
                "the model with name %s does not exist!" % fname)
        self.release()
        metadata = open(fname+"/metadata", "rb")
        self.n_components, self.n_features, self._converged, \
        self.n_iter_, self._lower_bound, self.__mkind,\
                         self.__mdtype = pickle.load(metadata)
        metadata.close()
        if dtype is not None:
            mdt = TypeUtil.to_numpy_dtype(self.__mdtype)
            if dtype != mdt:
                raise ValueError("load: type mismatches detected!" + \
                                 "expected type: " + str(mdt) + \
                                 "; given type: " + str(dtype))
        self.__mid = ModelID.get()
        GLM.load(self.__mid, self.__mkind, self.__mdtype, fname+"/model")
        return self        
        
    def save(self, fname):
        """
        NAME: save
        """
        if self.__mid is not None:
            if os.path.exists(fname):
                raise ValueError(\
                    "another model with %s name already exists!" % fname)
            else:
                os.makedirs(fname)
            GLM.save(self.__mid, self.__mkind, self.__mdtype, fname+"/model")
            metadata = open(fname+"/metadata", "wb")
            pickle.dump((self.n_components, self.n_features, 
                         self.converged_, self.n_iter_, self.lower_bound_, \
                         self.__mkind, self.__mdtype), metadata)
            metadata.close()
        else:
            raise ValueError(\
                "save: the requested model might have been released!")        

    def debug_print(self):
        """
        NAME: debug_print
        """
        if self.__mid is not None:
            GLM.debug_print(self.__mid, self.__mkind, self.__mdtype)        
        
    
    def release(self):
        """
        NAME: release
        """
        if self.__mid is not None:
            GLM.release(self.__mid, self.__mkind, self.__mdtype)
            self.__mid = None
            self._weights = None
            self._means = None
            self._covariances = None
            self._converged = None
            self.n_iter_ = None
            self._lower_bound = None        

    def __del__(self):
        """
        NAME: __del__
        """
        if FrovedisServer.isUP():
            self.release()        