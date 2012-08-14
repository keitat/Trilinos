// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_EPETRAVECTOR_HPP
#define XPETRA_EPETRAVECTOR_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_Vector.hpp"
#include "Xpetra_EpetraMultiVector.hpp"
#include "Xpetra_EpetraMap.hpp" //TMP
#include "Xpetra_Utils.hpp"
#include "Xpetra_EpetraImport.hpp"
#include "Xpetra_EpetraExport.hpp"
#include "Xpetra_Exceptions.hpp"

#include <Epetra_Vector.h>

namespace Xpetra {

  // TODO: move that elsewhere
  Epetra_Vector & toEpetra(Vector<double, int, int> &);

  const Epetra_Vector & toEpetra(const Vector<double, int, int> &);
  //

  class EpetraVector
    : public virtual Vector<double,int,int>, public EpetraMultiVector
  {

    typedef double Scalar;
    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    using EpetraMultiVector::dot;          // overloading, not hiding
    using EpetraMultiVector::norm1;        // overloading, not hiding
    using EpetraMultiVector::norm2;        // overloading, not hiding
    using EpetraMultiVector::normInf;      // overloading, not hiding
    using EpetraMultiVector::normWeighted; // overloading, not hiding
    using EpetraMultiVector::meanValue;    // overloading, not hiding
    using EpetraMultiVector::replaceGlobalValue;    // overloading, not hiding
    using EpetraMultiVector::sumIntoGlobalValue;    // overloading, not hiding
    using EpetraMultiVector::replaceLocalValue;    // overloading, not hiding
    using EpetraMultiVector::sumIntoLocalValue;    // overloading, not hiding

    //! @name Constructor/Destructor Methods
    //@{

    //! Sets all vector entries to zero.
    explicit EpetraVector(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, bool zeroOut=true);

    //! Vector copy constructor.
    EpetraVector(const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &source);

    //! Destructor.
    virtual ~EpetraVector() { }

    //@}

    //! @name Post-construction modification routines
    //@{

    //! Replace current value at the specified location with specified value.
    void replaceGlobalValue(GlobalOrdinal globalRow, const Scalar &value);

    //! Adds specified value to existing value at the specified location.
    void sumIntoGlobalValue(GlobalOrdinal globalRow, const Scalar &value);

    //! Replace current value at the specified location with specified values.
    void replaceLocalValue(LocalOrdinal myRow, const Scalar &value);

    //! Adds specified value to existing value at the specified location.
    void sumIntoLocalValue(LocalOrdinal myRow, const Scalar &value);

    //@}

    //! @name Mathematical methods
    //@{

    //! Computes dot product of this Vector against input Vector x.
    Scalar dot(const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &a) const;

    //! Return 1-norm of this Vector.
    Teuchos::ScalarTraits< Scalar >::magnitudeType norm1() const;

    //! Compute 2-norm of this Vector.
    Teuchos::ScalarTraits< Scalar >::magnitudeType norm2() const;

    //! Compute Inf-norm of this Vector.
    Teuchos::ScalarTraits< Scalar >::magnitudeType normInf() const;

    //! Compute Weighted 2-norm (RMS Norm) of this Vector.
    Teuchos::ScalarTraits< Scalar >::magnitudeType normWeighted(const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &weights) const;

    //! Compute mean (average) value of this Vector.
    Scalar meanValue() const;

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to an FancyOStream object.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const;

    //@}

    //! @name Xpetra specific
    //@{

    //! EpetraMultiVector constructor to wrap a Epetra_Vector object
    EpetraVector(const Teuchos::RCP<Epetra_Vector> &vec) : EpetraMultiVector(vec) { } // TODO: removed const of Epetra::Vector

    //! Get the underlying Epetra vector
    Epetra_Vector * getEpetra_Vector() const {  return (*this->EpetraMultiVector::getEpetra_MultiVector())(0); }
    //RCP<Epetra_Vector> getEpetra_Vector() const { return this->EpetraMultiVector::getEpetra_MultiVector()->getVectorNonConst(0); }

    //@}
    
  }; // EpetraVector class

} // Xpetra namespace

#endif // XPETRA_EPETRAVECTOR_HPP
