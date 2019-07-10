// Copyright 2018 The Simons Foundation, Inc. - All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NETKET_ABSTRACTMACHINE_HPP
#define NETKET_ABSTRACTMACHINE_HPP

#include <complex>
#include <iosfwd>
#include <memory>
#include <vector>

#include <Python.h>
#include <Eigen/Core>

#include "Hilbert/abstract_hilbert.hpp"
#include "Utils/any.hpp"
#include "Utils/lookup.hpp"
#include "Utils/random_utils.hpp"

namespace netket {

/**
  Abstract class for Machines.
  This class prototypes the methods needed
  by a class satisfying the Machine concept.
*/
class AbstractMachine {
 public:
  using VectorType = Eigen::Matrix<Complex, Eigen::Dynamic, 1>;
  using MatrixType = Eigen::Matrix<Complex, Eigen::Dynamic, Eigen::Dynamic>;
  using RowMatrixType =
      Eigen::Matrix<Complex, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  using LookupType = Lookup<Complex>;
  using VectorRefType = Eigen::Ref<VectorType>;
  using VectorConstRefType = Eigen::Ref<const VectorType>;
  using VisibleConstType = Eigen::Ref<const Eigen::VectorXd>;
  using VisibleType = Eigen::VectorXd;
  using RealVectorType = Eigen::Matrix<double, Eigen::Dynamic, 1>;
  using RealMatrixType = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
  using RealRowMatrixType =
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  using RealVectorConstRefType = Eigen::Ref<const RealVectorType>;

  /**
  Member function returning the number of variational parameters.
  @return Number of variational parameters in the Machine.
  */
  virtual int Npar() const = 0;

  /**
  Member function returning the current set of parameters in the machine.
  @return Current set of variational parameters of the machine.
  */
  virtual VectorType GetParameters() = 0;

  /**
  Member function setting the current set of parameters in the machine.
  */
  virtual void SetParameters(VectorConstRefType pars) = 0;

  /**
  Member function providing a random initialization of the parameters.
  @param sigma is the variance of the gaussian.
  @param seed is the seed of the random number generator. If seed is `nullopt`,
  one is generated using `std::random_device`.
  */
  virtual void InitRandomPars(double sigma, nonstd::optional<unsigned> seed);

  /**
  Member function returning the number of visible units.
  @return Number of visible units in the Machine.
  */
  virtual int Nvisible() const = 0;

  /**
   * Computes logarithm of the wave function for a batch of visible
   * configurations.
   *
   * @param v a matrix of size `batch_size x Nvisible()`. Each row of the matrix
   * is a visible configuration.
   */
  virtual void LogVal(Eigen::Ref<const RealRowMatrixType> v,
                      Eigen::Ref<VectorType> out, const any &cache = any{});

  virtual VectorType LogVal(Eigen::Ref<const RealRowMatrixType> v,
                            const any &cache = any{});

  virtual void DerLog(Eigen::Ref<const RealRowMatrixType> v,
                      Eigen::Ref<RowMatrixType> out, const any &cache = any{});

  virtual RowMatrixType DerLog(Eigen::Ref<const RealRowMatrixType> v,
                               const any &cache = any{});

  /**
  Member function computing the logarithm of the wave function for a given
  visible vector. Given the current set of parameters, this function should
  comput the value of the logarithm of the wave function from scratch.
  @param v a constant reference to a visible configuration.
  @return Logarithm of the wave function.
  */
  // virtual Complex LogValSingle(VisibleConstType v) = 0;

  /**
  Member function computing the logarithm of the wave function for a given
  visible vector. Given the current set of parameters, this function should
  comput the value of the logarithm of the wave function using the information
  provided in the look-up table, to speed up the computation.
  @param v a constant reference to a visible configuration.
  @param lt a constant eference to the look-up table.
  @return Logarithm of the wave function.
  */
  virtual Complex LogValSingle(VisibleConstType v, const any &lt = any{}) = 0;

  // This is a hack!
  //
  // Old and new (with batches) interfaces for LogVal overlap which makes some
  // calls to LogVal ambigous. Here, we catch such calls and redirect them to
  // the old function.
  //
  // This function should be removed once we get rid of the old interface.
  // Complex LogVal(VisibleType const &v, const any &lt) {
  //   return LogVal(VisibleConstType{v}, lt);
  // }

  /**
  Member function initializing the look-up tables.
  If needed, a Machine can make use of look-up tables
  to speed up some critical functions. For example,
  to speed up the calculation of wave-function ratios.
  The state of a look-up table depends on the visible units.
  This function should initialize the look-up tables
  making sure that memory in the table is also being allocated.
  @param v a constant reference to the visible configuration.
  @param lt a reference to the look-up table to be initialized.
  */
  virtual any InitLookup(VisibleConstType v) = 0;

  /**
  Member function updating the look-up tables.
  If needed, a Machine can make use of look-up tables
  to speed up some critical functions. For example,
  to speed up the calculation of wave-function ratios.
  The state of a look-up table depends on the visible units.
  This function should update the look-up tables
  when the state of visible units is changed according
  to the information stored in toflip and newconf
  @param v a constant reference to the current visible configuration.
  @param tochange a constant reference to a vector containing the indeces of the
  units to be modified.
  @param newconf a constant reference to a vector containing the new values of
  the visible units: here newconf(i)=v'(tochange(i)), where v' is the new
  visible state.
  @param lt a reference to the look-up table to be updated.
  */
  virtual void UpdateLookup(VisibleConstType v,
                            const std::vector<int> &tochange,
                            const std::vector<double> &newconf, any &lt) = 0;

  /**
  Member function computing the difference between the logarithm of the
  wave-function computed at different values of the visible units (v, and a set
  of v').
  @param v a constant reference to the current visible configuration.
  @param tochange a constant reference to a vector containing the indeces of the
  units to be modified.
  @param newconf a constant reference to a vector containing the new values of
  the visible units: here for each v', newconf(i)=v'(tochange(i)), where v' is
  the new visible state.
  @return A vector containing, for each v', log(Psi(v')) - log(Psi(v))
  */
  virtual VectorType LogValDiff(
      VisibleConstType v, const std::vector<std::vector<int>> &tochange,
      const std::vector<std::vector<double>> &newconf) = 0;

  /**
  Member function computing the difference between the logarithm of the
  wave-function computed at different values of the visible units (v, and a
  single v'). This version uses the look-up tables to speed-up the calculation.
  @param v a constant reference to the current visible configuration.
  @param tochange a constant reference to a vector containing the indeces of the
  units to be modified.
  @param newconf a constant reference to a vector containing the new values of
  the visible units: here newconf(i)=v'(tochange(i)), where v' is the new
  visible state.
  @param lt a constant eference to the look-up table.
  @return The value of log(Psi(v')) - log(Psi(v))
  */
  virtual Complex LogValDiff(VisibleConstType v,
                             const std::vector<int> &tochange,
                             const std::vector<double> &newconf,
                             const any &lt) = 0;

  /**
  Member function computing the derivative of the logarithm of the wave function
  for a given visible vector.
  @param v a constant reference to a visible configuration.
  @return Derivatives of the logarithm of the wave function with respect to the
  set of parameters.
  */
  virtual VectorType DerLogSingle(VisibleConstType v,
                                  const any &cache = any{}) = 0;

  /**
  Member function computing the derivative of the logarithm of the wave function
  for a given visible vector. This specialized version, if implemented, should
  make use of the Lookup table to speed up the calculation.
  @param v a constant reference to a visible configuration.
  @return Derivatives of the logarithm of the wave function with respect to the
  set of parameters.
  */
  // virtual VectorType DerLogSingle(VisibleConstType v, const any & /*lt*/) {
  //   return DerLog(v);
  // }

  // This is a hack!
  //
  // Old and new (with batches) interfaces for DerLog overlap which makes some
  // calls to DerLog ambigous. Here, we catch such calles and redirect them to
  // the old function.
  //
  // This function should be removed once we get rid of the old interface.
  // VectorType DerLog(VisibleType const &v, const any &lt) {
  //   return DerLog(VisibleConstType{v}, lt);
  // }

  /**
  Member function computing O_k(v'), the derivative of
  the logarithm of the wave function at an update visible state v', given the
  current value at v. Specialized versions use the look-up tables to speed-up
  the calculation, otherwise it is computed from scratch.
  @param v a constant reference to the current visible configuration.
  @param tochange a constant reference to a vector containing the indeces of the
  units to be modified.
  @param newconf a constant reference to a vector containing the new values of
  the visible units: here newconf(i)=v'(tochange(i)), where v' is the new
  visible state.
  @param lt a constant eference to the look-up table.
  @return The value of DerLog(v')
  */
  virtual VectorType DerLogChanged(VisibleConstType v,
                                   const std::vector<int> &tochange,
                                   const std::vector<double> &newconf);

  virtual bool IsHolomorphic() const noexcept = 0;

  virtual PyObject *StateDict() const {
    throw std::runtime_error{"Not implemented!"};
  }

  virtual PyObject *StateDict() {
    throw std::runtime_error{"Not implemented!"};
  }

  virtual void StateDict(PyObject *state) {
    throw std::runtime_error{"Not implemented!"};
  }

  virtual void Save(const std::string &filename) const = 0;
  virtual void Load(const std::string &filename) = 0;

  const AbstractHilbert &GetHilbert() const { return *hilbert_; };
  std::shared_ptr<const AbstractHilbert> GetHilbertShared() const {
    return hilbert_;
  };

  virtual ~AbstractMachine() = default;

 protected:
  AbstractMachine(std::shared_ptr<const AbstractHilbert> hilbert)
      : hilbert_(std::move(hilbert)) {}

 private:
  std::shared_ptr<const AbstractHilbert> hilbert_;
};

template <class LogValFn>
struct BatchedLogVal {
  template <class T>
  using RowMatrix =
      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

  LogValFn fn;

  void operator()(Eigen::Ref<const RowMatrix<double>> x,
                  Eigen::Ref<Eigen::VectorXcd> out) {
    if (x.rows() == 1 || x.cols() == 1) {
      if (out.size() != 1) {
        std::ostringstream msg;
        msg << "output tensor has wrong shape: [" << out.size()
            << "]; expected [1]";
        throw InvalidInputError{msg.str()};
      }
      out(0) = fn(Eigen::Ref<const Eigen::VectorXd>{x});
    } else {
      if (out.size() != x.rows()) {
        std::ostringstream msg;
        msg << "output tensor has wrong shape: [" << out.size()
            << "]; expected [" << x.rows() << "]";
        throw InvalidInputError{msg.str()};
      }
      for (auto j = Index{0}; j < x.rows(); ++j) {
        out(j) = fn(Eigen::Ref<const Eigen::VectorXd>{x.row(j)});
      }
    }
  }
};

template <class LogValFn>
struct BatchedDerLog {
  template <class T>
  using RowMatrix =
      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

  LogValFn fn;

  void operator()(Eigen::Ref<const RowMatrix<double>> x,
                  Eigen::Ref<RowMatrix<Complex>> out) {
    if (x.rows() == 1 || x.cols() == 1) {
      if (out.rows() != 1 && out.cols() != 1) {
        std::ostringstream msg;
        msg << "output tensor has wrong shape: [" << out.rows() << ", "
            << out.cols() << "]; expected [1, ?] or [?, 1]";
        throw InvalidInputError{msg.str()};
      }
      fn(Eigen::Ref<const Eigen::VectorXd>{x},
         Eigen::Map<Eigen::VectorXcd>{out.data(), out.size()});
    } else {
      if (out.rows() != x.rows()) {
        std::ostringstream msg;
        msg << "output tensor has wrong shape: [" << out.rows() << ", "
            << out.cols() << "]; expected [" << x.rows() << ", ?]";
        throw InvalidInputError{msg.str()};
      }
      for (auto j = Index{0}; j < x.rows(); ++j) {
        fn(Eigen::Ref<const Eigen::VectorXd>{x.row(j)},
           Eigen::Ref<Eigen::VectorXcd>{out.row(j)});
      }
    }
  }
};

template <class Fn>
auto MakeBatchedLogVal(Fn &&fn) noexcept(noexcept(
    BatchedLogVal<typename std::conditional<
        std::is_lvalue_reference<Fn>::value, Fn,
        typename std::remove_reference<Fn>::type>::type>{std::declval<Fn &&>()}

    ))
    -> BatchedLogVal<typename std::conditional<
        std::is_lvalue_reference<Fn>::value, Fn,
        typename std::remove_reference<Fn>::type>::type> {
  return BatchedLogVal<typename std::conditional<
      std::is_lvalue_reference<Fn>::value, Fn,
      typename std::remove_reference<Fn>::type>::type>{std::forward<Fn>(fn)};
}

template <class Fn>
auto MakeBatchedDerLog(Fn &&fn) noexcept(noexcept(
    BatchedDerLog<typename std::conditional<
        std::is_lvalue_reference<Fn>::value, Fn,
        typename std::remove_reference<Fn>::type>::type>{std::declval<Fn &&>()}

    ))
    -> BatchedDerLog<typename std::conditional<
        std::is_lvalue_reference<Fn>::value, Fn,
        typename std::remove_reference<Fn>::type>::type> {
  return BatchedDerLog<typename std::conditional<
      std::is_lvalue_reference<Fn>::value, Fn,
      typename std::remove_reference<Fn>::type>::type>{std::forward<Fn>(fn)};
}

}  // namespace netket

#endif  // NETKET_ABSTRACTMACHINE_HPP
