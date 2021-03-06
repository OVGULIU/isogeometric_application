//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 15 Nov 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_BSPLINES_FESPACE_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_BSPLINES_FESPACE_H_INCLUDED

// System includes
#include <vector>

// External includes
#include <boost/array.hpp>

// Project includes
#include "includes/define.h"
#include "containers/array_1d.h"
#include "custom_utilities/bezier_utils.h"
#include "custom_utilities/bspline_utils.h"
#include "custom_utilities/fespace.h"
#include "custom_utilities/nurbs/knot_array_1d.h"
#include "custom_utilities/nurbs/bsplines_indexing_utility.h"
#include "custom_utilities/nurbs/cell_manager_1d.h"
#include "custom_utilities/nurbs/cell_manager_2d.h"
#include "custom_utilities/nurbs/cell_manager_3d.h"

// #define DEBUG_GEN_CELL

namespace Kratos
{

/**
This class represents the FESpace for a single BSplines patch defined over parametric domain.
 */
template<int TDim>
class BSplinesFESpace : public FESpace<TDim>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(BSplinesFESpace);

    /// Type definition
    typedef FESpace<TDim> BaseType;
    typedef KnotArray1D<double> knot_container_t;
    typedef typename knot_container_t::knot_t knot_t;
    typedef typename BaseType::cell_container_t cell_container_t;

    /// Default constructor
    BSplinesFESpace() : BaseType() {}

    /// Destructor
    virtual ~BSplinesFESpace() {}

    /// Helper to create new BSplinesFESpace pointer
    static BSplinesFESpace<TDim>::Pointer Create()
    {
        return BSplinesFESpace<TDim>::Pointer(new BSplinesFESpace());
    }

    /// Get the order of the BSplines patch in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        if (i >= TDim) return 0;
        else return mOrders[i];
    }

    /// Get the number of control points of the BSplines in specific direction
    const std::size_t Number(const std::size_t& i) const {return mNumbers[i];}

    /// Get the number of basis functions defined over the BSplines
    virtual const std::size_t TotalNumber() const
    {
        std::size_t Number = 1;
        for (std::size_t i = 0; i < TDim; ++i)
            Number *= mNumbers[i];
        return Number;
    }

    /// Get the string representing the type of the patch
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string representing the type of the patch
    static std::string StaticType()
    {
        std::stringstream ss;
        ss << "BSplinesFESpace" << TDim << "D";
        return ss.str();
    }

    /// Set the knot vector in direction i.
    void SetKnotVector(const std::size_t& i, const knot_container_t& p_knot_vector)
    {
        mKnotVectors[i] = p_knot_vector;
    }

    /// Create and set the knot vector in direction i.
    void SetKnotVector(const std::size_t& i, const std::vector<double>& values)
    {
        if (i >= TDim)
        {
            KRATOS_THROW_ERROR(std::logic_error, "Invalid dimension", "")
        }
        else
        {
            mKnotVectors[i].clear();
            for (std::size_t j = 0; j < values.size(); ++j)
                mKnotVectors[i].pCreateKnot(values[j]);
        }
    }

    /// Get the knot vector in i-direction
    const knot_container_t& KnotVector(const std::size_t& i) const {return mKnotVectors[i];}

    /// Set the BSplines information in the direction i
    void SetInfo(const std::size_t& i, const std::size_t& Number, const std::size_t& Order)
    {
        mOrders[i] = Order;
        mNumbers[i] = Number;
    }

    /// Validate the BSplinesFESpace
    virtual bool Validate() const
    {
        for (std::size_t i = 0; i < TDim; ++i)
        {
            if (mKnotVectors[i].size() != mNumbers[i] + mOrders[i] + 1)
            {
                KRATOS_THROW_ERROR(std::logic_error, "The knot vector is incompatible at dimension", i)
                return false;
            }
        }

        return BaseType::Validate();
    }

    /// Get the values of the basis function i at point xi
    virtual double GetValue(const std::size_t& i, const std::vector<double>& xi) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "GetValue is not implemented for dimension", TDim)
    }

    /// Get the values of the basis functions at point xi
    virtual std::vector<double> GetValue(const std::vector<double>& xi) const
    {
        KRATOS_THROW_ERROR(std::logic_error, "GetValue is not implemented for dimension", TDim)
    }

    /// Compare between two BSplines patches in terms of parametric information
    virtual bool IsCompatible(const FESpace<TDim>& rOtherFESpace) const
    {
        if (rOtherFESpace.Type() != Type())
        {
            KRATOS_WATCH(rOtherFESpace.Type())
            KRATOS_WATCH(Type())
            std::cout << "WARNING!!! the other patch type is not " << Type() << std::endl;
            return false;
        }

        const BSplinesFESpace<TDim>& rOtherBSplinesFESpace = dynamic_cast<const BSplinesFESpace<TDim>&>(rOtherFESpace);

        // compare the knot vectors and order information
        for (std::size_t i = 0; i < TDim; ++i)
        {
            if (!(this->Number(i)) == rOtherBSplinesFESpace.Number(i))
                return false;
            if (!(this->Order(i)) == rOtherBSplinesFESpace.Order(i))
                return false;
            if (!(this->KnotVector(i) == rOtherBSplinesFESpace.KnotVector(i)))
                return false;
        }

        return true;
    }

    /// Extract the index of the functions on the boundary
    virtual std::vector<std::size_t> ExtractBoundaryFunctionIndices(const BoundarySide& side) const
    {
        std::vector<std::size_t> func_indices;

        if (side == _LEFT_)
        {
            if (TDim == 1)
            {
                func_indices.resize(1);
                func_indices[0] = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index1D(1, this->Number(0))];
            }
            else if (TDim == 2)
            {
                func_indices.resize(this->Number(1));
                for (std::size_t j = 0; j < this->Number(1); ++j)
                    func_indices[BSplinesIndexingUtility::Index1D(j+1, this->Number(1))]
                        = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(1, j+1, this->Number(0), this->Number(1))];
            }
            else if (TDim == 3)
            {
                func_indices.resize(this->Number(1)*this->Number(2));
                for (std::size_t j = 0; j < this->Number(1); ++j)
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                        func_indices[BSplinesIndexingUtility::Index2D(j+1, k+1, this->Number(1), this->Number(2))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(1, j+1, k+1, this->Number(0), this->Number(1), this->Number(2))];
            }
        }
        else if (side == _RIGHT_)
        {
            if (TDim == 1)
            {
                func_indices.resize(1);
                func_indices[0] = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index1D(this->Number(0), this->Number(0))];
            }
            else if (TDim == 2)
            {
                func_indices.resize(this->Number(1));
                for (std::size_t j = 0; j < this->Number(1); ++j)
                    func_indices[BSplinesIndexingUtility::Index1D(j+1, this->Number(1))]
                        = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(this->Number(0), j+1, this->Number(0), this->Number(1))];
            }
            else if (TDim == 3)
            {
                func_indices.resize(this->Number(1)*this->Number(2));
                for (std::size_t j = 0; j < this->Number(1); ++j)
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                        func_indices[BSplinesIndexingUtility::Index2D(j+1, k+1, this->Number(1), this->Number(2))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(this->Number(0), j+1, k+1, this->Number(0), this->Number(1), this->Number(2))];
            }
        }
        else if (side == _BOTTOM_)
        {
            if (TDim == 2)
            {
                func_indices.resize(this->Number(0));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    func_indices[BSplinesIndexingUtility::Index1D(i+1, this->Number(0))]
                        = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(i+1, 1, this->Number(0), this->Number(1))];
            }
            else if (TDim == 3)
            {
                func_indices.resize(this->Number(0)*this->Number(1));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    for (std::size_t j = 0; j < this->Number(1); ++j)
                        func_indices[BSplinesIndexingUtility::Index2D(i+1, j+1, this->Number(0), this->Number(1))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, j+1, 1, this->Number(0), this->Number(1), this->Number(2))];
            }
        }
        else if (side == _TOP_)
        {
            if (TDim == 2)
            {
                func_indices.resize(this->Number(0));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    func_indices[BSplinesIndexingUtility::Index1D(i+1, this->Number(0))]
                        = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(i+1, this->Number(1), this->Number(0), this->Number(1))];
            }
            else if (TDim == 3)
            {
                func_indices.resize(this->Number(0)*this->Number(1));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    for (std::size_t j = 0; j < this->Number(1); ++j)
                        func_indices[BSplinesIndexingUtility::Index2D(i+1, j+1, this->Number(0), this->Number(1))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, j+1, this->Number(2), this->Number(0), this->Number(1), this->Number(2))];
            }
        }
        else if (side == _FRONT_)
        {
            if (TDim == 3)
            {
                func_indices.resize(this->Number(0)*this->Number(2));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                        func_indices[BSplinesIndexingUtility::Index2D(i+1, k+1, this->Number(0), this->Number(2))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, 1, k+1, this->Number(0), this->Number(1), this->Number(2))];
            }
        }
        else if (side == _BACK_)
        {
            if (TDim == 3)
            {
                func_indices.resize(this->Number(0)*this->Number(2));
                for (std::size_t i = 0; i < this->Number(0); ++i)
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                        func_indices[BSplinesIndexingUtility::Index2D(i+1, k+1, this->Number(0), this->Number(2))]
                            = BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, this->Number(1), k+1, this->Number(0), this->Number(1), this->Number(2))];
            }
        }

        return func_indices;
    }

    /// Assign the index for the functions on the boundary
    virtual void AssignBoundaryFunctionIndices(const BoundarySide& side, const std::vector<std::size_t>& func_indices)
    {
        if (side == _LEFT_)
        {
            if (TDim == 1)
            {
                if (func_indices[0] != -1)
                    BaseType::mFunctionsIds[BSplinesIndexingUtility::Index1D(1, this->Number(0))] = func_indices[0];
            }
            else if (TDim == 2)
            {
                for (std::size_t j = 0; j < this->Number(1); ++j)
                {
                    const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index1D(j+1, this->Number(1))];
                    if (aux != -1)
                        BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(1, j+1, this->Number(0), this->Number(1))] = aux;
                }
            }
            else if (TDim == 3)
            {
                for (std::size_t j = 0; j < this->Number(1); ++j)
                {
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(j+1, k+1, this->Number(1), this->Number(2))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(1, j+1, k+1, this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
        else if (side == _RIGHT_)
        {
            if (TDim == 1)
            {
                if (func_indices[0] != -1)
                    BaseType::mFunctionsIds[BSplinesIndexingUtility::Index1D(this->Number(0), this->Number(0))] = func_indices[0];
            }
            else if (TDim == 2)
            {
                for (std::size_t j = 0; j < this->Number(1); ++j)
                {
                    const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index1D(j+1, this->Number(1))];
                    if (aux != -1)
                        BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(this->Number(0), j+1, this->Number(0), this->Number(1))] = aux;
                }
            }
            else if (TDim == 3)
            {
                for (std::size_t j = 0; j < this->Number(1); ++j)
                {
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(j+1, k+1, this->Number(1), this->Number(2))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(this->Number(0), j+1, k+1, this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
        else if (side == _BOTTOM_)
        {
            if (TDim == 2)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index1D(i+1, this->Number(0))];
                    if (aux != -1)
                        BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(i+1, 1, this->Number(0), this->Number(1))] = aux;
                }
            }
            else if (TDim == 3)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    for (std::size_t j = 0; j < this->Number(1); ++j)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(i+1, j+1, this->Number(0), this->Number(1))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, j+1, 1, this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
        else if (side == _TOP_)
        {
            if (TDim == 2)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index1D(i+1, this->Number(0))];
                    if (aux != -1)
                        BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(i+1, this->Number(1), this->Number(0), this->Number(1))] = aux;
                }
            }
            else if (TDim == 3)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    for (std::size_t j = 0; j < this->Number(1); ++j)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(i+1, j+1, this->Number(0), this->Number(1))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, j+1, this->Number(2), this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
        else if (side == _FRONT_)
        {
            if (TDim == 3)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(i+1, k+1, this->Number(0), this->Number(2))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, 1, k+1, this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
        else if (side == _BACK_)
        {
            if (TDim == 3)
            {
                for (std::size_t i = 0; i < this->Number(0); ++i)
                {
                    for (std::size_t k = 0; k < this->Number(2); ++k)
                    {
                        const std::size_t& aux = func_indices[BSplinesIndexingUtility::Index2D(i+1, k+1, this->Number(0), this->Number(2))];
                        if (aux != -1)
                            BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, this->Number(1), k+1, this->Number(0), this->Number(1), this->Number(2))] = aux;
                    }
                }
            }
        }
    }

    /// Construct the boundary patch based on side
    virtual typename FESpace<TDim-1>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    {
        typename BSplinesFESpace<TDim-1>::Pointer pBFESpace = typename BSplinesFESpace<TDim-1>::Pointer(new BSplinesFESpace<TDim-1>());

        // assign the knot vectors
        if (TDim == 2)
        {
            if (side == _LEFT_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(1));
                pBFESpace->SetInfo(0, Number(1), Order(1));
            }
            else if (side == _RIGHT_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(1));
                pBFESpace->SetInfo(0, Number(1), Order(1));
            }
            else if (side == _TOP_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetInfo(0, Number(0), Order(0));
            }
            else if (side == _BOTTOM_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetInfo(0, Number(0), Order(0));
            }
        }
        else if (TDim == 3)
        {
            if (side == _LEFT_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(1));
                pBFESpace->SetKnotVector(1, KnotVector(2));
                pBFESpace->SetInfo(0, Number(1), Order(1));
                pBFESpace->SetInfo(1, Number(2), Order(2));
            }
            else if (side == _RIGHT_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(1));
                pBFESpace->SetKnotVector(1, KnotVector(2));
                pBFESpace->SetInfo(0, Number(1), Order(1));
                pBFESpace->SetInfo(1, Number(2), Order(2));
            }
            else if (side == _TOP_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetKnotVector(1, KnotVector(1));
                pBFESpace->SetInfo(0, Number(0), Order(0));
                pBFESpace->SetInfo(1, Number(1), Order(1));
            }
            else if (side == _BOTTOM_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetKnotVector(1, KnotVector(1));
                pBFESpace->SetInfo(0, Number(0), Order(0));
                pBFESpace->SetInfo(1, Number(1), Order(1));
            }
            else if (side == _FRONT_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetKnotVector(1, KnotVector(2));
                pBFESpace->SetInfo(0, Number(0), Order(0));
                pBFESpace->SetInfo(1, Number(2), Order(2));
            }
            else if (side == _BACK_)
            {
                pBFESpace->SetKnotVector(0, KnotVector(0));
                pBFESpace->SetKnotVector(1, KnotVector(2));
                pBFESpace->SetInfo(0, Number(0), Order(0));
                pBFESpace->SetInfo(1, Number(2), Order(2));
            }
        }

        // transfer the function indices
        std::vector<std::size_t> b_func_indices = this->ExtractBoundaryFunctionIndices(side);
        pBFESpace->ResetFunctionIndices(b_func_indices);

        return pBFESpace;
    }

    /// Create the cell manager for all the cells in the support domain of the BSplinesFESpace
    virtual typename cell_container_t::Pointer ConstructCellManager() const
    {
        typename cell_container_t::Pointer pCellManager;

        if (TDim == 1)
        {
            pCellManager = typename cell_container_t::Pointer(new CellManager1D<Cell>());

            // firstly compute the Bezier extraction operator on the patch
            std::vector<Matrix> C;
            int ne1;
            // BezierUtils::bezier_extraction_2d(C, ne1, ne2, this->KnotVector(0), this->KnotVector(1), this->Order(0), this->Order(1));
            BezierUtils::bezier_extraction_1d(C, ne1, this->KnotVector(0), this->Order(0));

            #ifdef DEBUG_GEN_CELL
            KRATOS_WATCH(ne1)
            KRATOS_WATCH(C.size())
            KRATOS_WATCH(C[0].size1())
            KRATOS_WATCH(C[0].size2())
            #endif

            // construct cells and add to the manager
            std::size_t n1 = this->Number(0);
            std::size_t p1 = this->Order(0);
            std::size_t i, j, k, l, b1 = p1+1, tmp, mul1, sum_mul1 = 0, id1, id;
            std::size_t cnt = 0; // cell counter

            for (i = 0; i < ne1; ++i)
            {
                // check the multiplicity
                tmp = b1;
                while (b1 <= (n1 + p1 + 1) && this->KnotVector(0)[b1] == this->KnotVector(0)[b1-1]) ++b1;
                mul1 = b1 - tmp + 1;
                b1 = b1 + 1;
                sum_mul1 = sum_mul1 + (mul1 - 1);

                std::vector<std::size_t> anchors;
                anchors.reserve(p1+1);
                for (k = 0; k < p1+1; ++k)
                {
                    id1 = i + k + sum_mul1;
                    id = id1; // this is the local id
                    anchors.push_back(id);
                }

                // add the cell
                std::tuple<knot_t, knot_t> span1 = this->KnotVector(0).span(i+1);
                Cell::Pointer p_cell = Cell::Pointer(new Cell(cnt, std::get<0>(span1), std::get<1>(span1)));
                double W = 1.0; // here we set to one because B-Splines space does not have weight
                for (std::size_t r = 0; r < (p1+1); ++r)
                    p_cell->AddAnchor(BaseType::FunctionIndices()[anchors[r]], W, row(C[cnt], r));
                pCellManager->insert(p_cell);
                ++cnt;
            }
        }
        else if (TDim == 2)
        {
            pCellManager = typename cell_container_t::Pointer(new CellManager2D<Cell>());

            // firstly compute the Bezier extraction operator on the patch
            std::vector<Matrix> C;
            int ne1, ne2;
            // BezierUtils::bezier_extraction_2d(C, ne1, ne2, this->KnotVector(0), this->KnotVector(1), this->Order(0), this->Order(1));
            BezierUtils::bezier_extraction_2d(C, ne2, ne1, this->KnotVector(1), this->KnotVector(0), this->Order(1), this->Order(0)); // we rotate the order of input

            #ifdef DEBUG_GEN_CELL
            KRATOS_WATCH(ne1)
            KRATOS_WATCH(ne2)
            KRATOS_WATCH(C.size())
            KRATOS_WATCH(C[0].size1())
            KRATOS_WATCH(C[0].size2())
            #endif

            // construct cells and add to the manager
            std::size_t n1 = this->Number(0);
            std::size_t n2 = this->Number(1);
            std::size_t p1 = this->Order(0);
            std::size_t p2 = this->Order(1);
            std::size_t i, j, k, l, b1 = p1+1, b2, tmp, mul1, mul2, sum_mul1 = 0, sum_mul2 = 0, id1, id2, id;
            std::size_t cnt = 0; // cell counter

            for (i = 0; i < ne1; ++i)
            {
                // check the multiplicity
                tmp = b1;
                while (b1 <= (n1 + p1 + 1) && this->KnotVector(0)[b1] == this->KnotVector(0)[b1-1]) ++b1;
                mul1 = b1 - tmp + 1;
                b1 = b1 + 1;
                sum_mul1 = sum_mul1 + (mul1 - 1);

                b2 = p2 + 1;
                sum_mul2 = 0;
                for (j = 0; j < ne2; ++j)
                {
                    // check the multiplicity
                    tmp = b2;
                    while (b2 <= (n2 + p2 + 1) && this->KnotVector(1)[b2] == this->KnotVector(1)[b2-1]) ++b2;
                    mul2 = b2 - tmp + 1;
                    b2 = b2 + 1;
                    sum_mul2 = sum_mul2 + (mul2 - 1);

                    std::vector<std::size_t> anchors;
                    anchors.reserve((p1+1)*(p2+1));
                    for (k = 0; k < p1+1; ++k)
                    {
                        for (l = 0; l < p2+1; ++l)
                        {
                            id1 = i + k + sum_mul1;
                            id2 = j + l + sum_mul2;
                            id = id1 + id2*n1; // this is the local id
                            anchors.push_back(id);
                        }
                    }

                    // add the cell
                    std::tuple<knot_t, knot_t> span1 = this->KnotVector(0).span(i+1);
                    std::tuple<knot_t, knot_t> span2 = this->KnotVector(1).span(j+1);
                    Cell::Pointer p_cell = Cell::Pointer(new Cell(cnt, std::get<0>(span1), std::get<1>(span1), std::get<0>(span2), std::get<1>(span2)));
                    double W = 1.0; // here we set to one because B-Splines space does not have weight
                    for (std::size_t r = 0; r < (p1+1)*(p2+1); ++r)
                        p_cell->AddAnchor(BaseType::FunctionIndices()[anchors[r]], W, row(C[cnt], r));
                    pCellManager->insert(p_cell);
                    ++cnt;
                }
            }
        }
        else if(TDim == 3)
        {
            pCellManager = typename cell_container_t::Pointer(new CellManager3D<Cell>());

            // firstly compute the Bezier extraction operator on the patch
            std::vector<Matrix> C;
            int ne1, ne2, ne3;
            // BezierUtils::bezier_extraction_3d(C, ne1, ne2, ne3,
            //     this->KnotVector(0), this->KnotVector(1), this->KnotVector(2),
            //     this->Order(0), this->Order(1), this->Order(2));
            BezierUtils::bezier_extraction_3d(C, ne3, ne2, ne1,
                this->KnotVector(2), this->KnotVector(1), this->KnotVector(0),
                this->Order(2), this->Order(1), this->Order(0)); // we rotate the order of input

            // construct cells and add to the manager
            std::size_t n1 = this->Number(0);
            std::size_t n2 = this->Number(1);
            std::size_t n3 = this->Number(2);
            std::size_t p1 = this->Order(0);
            std::size_t p2 = this->Order(1);
            std::size_t p3 = this->Order(2);
            std::size_t i, j, k, l, u, v, w, b1 = p1+1, b2, b3, tmp, mul1, mul2, mul3;
            std::size_t sum_mul1 = 0, sum_mul2 = 0, sum_mul3 = 0, id1, id2, id3, id;
            std::size_t cnt = 0; // cell counter

            for (i = 0; i < ne1; ++i)
            {
                // check the multiplicity
                tmp = b1;
                while (b1 <= (n1 + p1 + 1) && this->KnotVector(0)[b1] == this->KnotVector(0)[b1-1]) ++b1;
                mul1 = b1 - tmp + 1;
                b1 = b1 + 1;
                sum_mul1 = sum_mul1 + (mul1 - 1);

                b2 = p2 + 1;
                sum_mul2 = 0;
                for (j = 0; j < ne2; ++j)
                {
                    // check the multiplicity
                    tmp = b2;
                    while (b2 <= (n2 + p2 + 1) && this->KnotVector(1)[b2] == this->KnotVector(1)[b2-1]) ++b2;
                    mul2 = b2 - tmp + 1;
                    b2 = b2 + 1;
                    sum_mul2 = sum_mul2 + (mul2 - 1);

                    b3 = p3 + 1;
                    sum_mul3 = 0;
                    for (k = 0; k < ne3; ++k)
                    {
                        // check the multiplicity
                        tmp = b3;
                        while (b3 <= (n3 + p3 + 1) && this->KnotVector(2)[b3] == this->KnotVector(2)[b3-1]) ++b3;
                        mul3 = b3 - tmp + 1;
                        b3 = b3 + 1;
                        sum_mul3 = sum_mul3 + (mul3 - 1);

                        std::vector<std::size_t> anchors;
                        anchors.reserve((p1+1)*(p2+1)*(p3+1));
                        for (u = 0; u < p1+1; ++u)
                        {
                            for (v = 0; v < p2+1; ++v)
                            {
                                for (v = 0; v < p2+1; ++v)
                                {
                                    for (w = 0; w < p3+1; ++w)
                                    {
                                        id1 = i + u + sum_mul1;
                                        id2 = j + v + sum_mul2;
                                        id3 = k + w + sum_mul3;
                                        id = id1 + (id2 + id3 * n2) * n1; // this is the local id
                                        anchors.push_back(id);
                                    }
                                }
                            }
                        }

                        // add the cell
                        std::tuple<knot_t, knot_t> span1 = this->KnotVector(0).span(i+1);
                        std::tuple<knot_t, knot_t> span2 = this->KnotVector(1).span(j+1);
                        std::tuple<knot_t, knot_t> span3 = this->KnotVector(2).span(k+1);
                        Cell::Pointer p_cell = Cell::Pointer(new Cell(cnt, std::get<0>(span1), std::get<1>(span1),
                                std::get<0>(span2), std::get<1>(span2), std::get<0>(span3), std::get<1>(span3)));
                        double W = 1.0; // here we set to one because B-Splines space does not have weight
                        for (std::size_t r = 0; r < (p1+1)*(p2+1)*(p3+1); ++r)
                            p_cell->AddAnchor(BaseType::FunctionIndices()[anchors[r]], W, row(C[cnt], r));
                        pCellManager->insert(p_cell);
                        ++cnt;
                    }
                }
            }
        }

        return pCellManager;
    }

    /// Overload assignment operator
    BSplinesFESpace<TDim>& operator=(const BSplinesFESpace<TDim>& rOther)
    {
        for (std::size_t dim = 0; dim < TDim; ++dim)
        {
            this->SetKnotVector(dim, rOther.KnotVector(dim));
            this->SetInfo(dim, rOther.Number(dim), rOther.Order(dim));
        }
        BaseType::operator=(rOther);
        return *this;
    }

    /// Clone this FESpace, this is a deep copy operation
    virtual typename FESpace<TDim>::Pointer Clone() const
    {
        typename BSplinesFESpace<TDim>::Pointer pNewFESpace = typename BSplinesFESpace<TDim>::Pointer(new BSplinesFESpace<TDim>());
        *pNewFESpace = *this;
        return pNewFESpace;
    }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << Type() << ", Addr = " << this << ", n = (";
        for (std::size_t i = 0; i < TDim; ++i)
            rOStream << " " << this->Number(i);
        rOStream << "), p = (";
        for (std::size_t i = 0; i < TDim; ++i)
            rOStream << " " << this->Order(i);
        rOStream << ")";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        for (std::size_t i = 0; i < TDim; ++i)
        {
            rOStream << " knot vector " << i << ":";
            for (std::size_t j = 0; j < mKnotVectors[i].size(); ++j)
                rOStream << " " << mKnotVectors[i].pKnotAt(j)->Value();
            rOStream << std::endl;
        }
        if (BaseType::mFunctionsIds.size() == this->TotalNumber())
        {
            rOStream << " Function Indices:";
            if (TDim == 1)
            {
                for (std::size_t i = 0; i < BaseType::mFunctionsIds.size(); ++i)
                    rOStream << " " << BaseType::mFunctionsIds[i];
            }
            else if (TDim == 2)
            {
                for (std::size_t j = 0; j < this->Number(1); ++j)
                {
                    for (std::size_t i = 0; i < this->Number(0); ++i)
                        rOStream << " " << BaseType::mFunctionsIds[BSplinesIndexingUtility::Index2D(i+1, j+1, this->Number(0), this->Number(1))];
                    rOStream << std::endl;
                }
            }
            else if (TDim == 3)
            {
                for (std::size_t k = 0; k < this->Number(2); ++k)
                {
                    for (std::size_t j = 0; j < this->Number(1); ++j)
                    {
                        for (std::size_t i = 0; i < this->Number(0); ++i)
                            rOStream << " " << BaseType::mFunctionsIds[BSplinesIndexingUtility::Index3D(i+1, j+1, k+1, this->Number(0), this->Number(1), this->Number(2))];
                        rOStream << std::endl;
                    }
                    rOStream << std::endl;
                }
            }
        }
    }

private:

    /**
     * internal data to construct the shape functions on the BSplines
     */
    boost::array<std::size_t, TDim> mOrders;
    boost::array<std::size_t, TDim> mNumbers;
    boost::array<knot_container_t, TDim> mKnotVectors;
};

/**
 * Template specific instantiation for null-D BSplines patch to terminate the compilation
 */
template<>
class BSplinesFESpace<0> : public FESpace<0>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(BSplinesFESpace);

    /// Type definition
    typedef FESpace<0> BaseType;
    typedef KnotArray1D<double> knot_container_t;
    typedef typename knot_container_t::knot_t knot_t;

    /// Default constructor
    BSplinesFESpace() : BaseType() {}

    /// Destructor
    virtual ~BSplinesFESpace() {}

    /// Get the order of the BSplines patch in specific direction
    virtual const std::size_t Order(const std::size_t& i) const {return 0;}

    /// Get the number of basis functions defined over the BSplines BSplinesFESpace
    virtual const std::size_t Number() const {return 0;}

    /// Get the string describing the type of the patch
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the patch
    static std::string StaticType()
    {
        return "BSplinesFESpace0D";
    }

    /// Set the knot vector in direction i.
    void SetKnotVector(const std::size_t& i, const knot_container_t& p_knot_vector)
    {}

    /// Set the BSplines information in the direction i
    void SetInfo(const std::size_t& i, const std::size_t& Number, const std::size_t& Order)
    {}

    /// Validate the BSplinesFESpace before using
    virtual bool Validate() const
    {
        return BaseType::Validate();
    }

    /// Compare between two BSplines patches in terms of parametric information
    virtual bool IsCompatible(const FESpace<0>& rOtherFESpace) const
    {
        if (rOtherFESpace.Type() != Type())
        {
            KRATOS_WATCH(rOtherFESpace.Type())
            KRATOS_WATCH(Type())
            std::cout << "WARNING!!! the other FESpace type is not " << Type() << std::endl;
            return false;
        }

        return true;
    }
};

/// output stream function
template<int TDim>
inline std::ostream& operator <<(std::ostream& rOStream, const BSplinesFESpace<TDim>& rThis)
{
    rOStream << "-------------Begin BSplinesFESpace Info-------------" << std::endl;
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    rOStream << std::endl;
    rOStream << "-------------End BSplinesFESpace Info-------------" << std::endl;
    return rOStream;
}

} // namespace Kratos.

#undef DEBUG_GEN_CELL

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_BSPLINES_FESPACE_H_INCLUDED defined
