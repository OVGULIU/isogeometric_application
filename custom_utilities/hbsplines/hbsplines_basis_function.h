//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 24 Nov 2017 $
//   Revision:            $Revision: 1.1 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_HBSPLINES_BASIS_FUNCTION_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_HBSPLINES_BASIS_FUNCTION_H_INCLUDED

// System includes
#include <ctime>
#include <cmath>
#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <set>
#include <list>

// External includes
#include <omp.h>
#include "boost/progress.hpp"
#include "boost/algorithm/string.hpp"

// Project includes
#include "includes/define.h"
#include "includes/ublas_interface.h"
#include "containers/data_value_container.h"
#include "custom_utilities/bezier_utils.h"
#include "custom_utilities/nurbs/knot.h"
#include "custom_utilities/nurbs/knot_array_1d.h"
#include "custom_utilities/control_point.h"
#include "custom_utilities/hbsplines/hb_cell.h"

namespace Kratos
{

template<int TDim>
struct HBSplinesBasisFunction_Helper
{
    /// Find the span of knot in the local knot vector
    /// Remarks: it will give the based-1 index
    ///          it only works if U is non-repeated
    template<typename TDataType, class TValuesContainerType>
    static std::size_t FindSpanLocal(const TDataType& Xi, const TValuesContainerType& U)
    {
        if(!U.empty())
        {
            if(Xi < U[0])
                return 0;

            if(Xi > U[U.size()-1])
                return U.size();

            for(std::size_t i = 0; i < U.size()-1; ++i)
                if(Xi >= U[i] && Xi < U[i + 1])
                    return i + 1;
        }

        return 0;
    }

    template<typename TVectorType, typename TIArrayType, typename TKnotContainerType, class TCellType>
    static void ComputeExtractionOperator(TVectorType& Crow, const TIArrayType& orders,
        const TKnotContainerType& local_knots, const TCellType& r_cell);
};

/**
Class represents a basis function in hierarchical B-Splines mesh.
Each basis function associates with a control point via CONTROL_POINT variable.
*/
template<int TDim>
class HBSplinesBasisFunction
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(HBSplinesBasisFunction);

    /// Type definition
    typedef ControlPoint<double> ControlPointType;
    typedef Knot<double>::Pointer knot_t;

    typedef HBSplinesBasisFunction::Pointer bf_t;
    typedef std::vector<bf_t> bf_container_t;
    typedef typename bf_container_t::iterator bf_iterator;
    typedef typename bf_container_t::const_iterator bf_const_iterator;

    typedef HBCell<HBSplinesBasisFunction<TDim> > CellType;
    typedef typename CellType::Pointer cell_t;
    typedef typename CellType::ConstPointer const_cell_t;
    typedef std::set<cell_t> cell_container_t;
    typedef typename cell_container_t::iterator cell_iterator;
    typedef typename cell_container_t::const_iterator cell_const_iterator;

    /// Default constructor
    HBSplinesBasisFunction(const std::size_t& Id, const std::size_t& Level) : mId(Id), mLevel(Level)
    {}

    /// Destructor
    ~HBSplinesBasisFunction()
    {}

    /**************************************************************************
                            MODIFICATION SUBROUTINES
    **************************************************************************/

    /// Get the number of children of this basis function
    std::size_t NumberOfChildren() const {return mpChilds.size();}

    /// Add a child which support this basis function
    void AddChild(bf_t p_bf, double RefinedCoefficient)
    {
        mpChilds.push_back(p_bf);
        mRefinedCoefficients[p_bf->Id()] = RefinedCoefficient;
    }

    /// Remove the cell from the list
    void RemoveChild(bf_t p_bf)
    {
        for(bf_iterator it = bf_begin(); it != bf_end(); ++it)
        {
            if(*it == p_bf)
            {
                mpChilds.erase(it);
                mRefinedCoefficients.erase(p_bf->Id());
                break;
            }
        }
    }

    /// Add a cell support this basis function to the list
    cell_iterator AddCell(cell_t p_cell)
    {
        for(cell_iterator it = cell_begin(); it != cell_end(); ++it)
            if(*it == p_cell)
                return it;
        return mpCells.insert(p_cell).first;
    }

    /// Remove the cell from the list
    void RemoveCell(cell_t p_cell)
    {
        for(cell_iterator it = cell_begin(); it != cell_end(); ++it)
        {
            if(*it == p_cell)
            {
                mpCells.erase(it);
                break;
            }
        }
    }

    /// Set the local knot vectors to this basis function
    void SetLocalKnotVectors(const int& dim, const std::vector<knot_t>& rpKnots)
    {
        mpLocalKnots[dim].clear();
        for(std::size_t i = 0; i < rpKnots.size(); ++i)
            mpLocalKnots[dim].push_back(rpKnots[i]);
    }

    /**************************************************************************
                            ACCESS SUBROUTINES
    **************************************************************************/

    /// Iterators to the child of this basis function
    bf_iterator bf_begin() {return mpChilds.begin();}
    bf_const_iterator bf_begin() const {return mpChilds.begin();}
    bf_iterator bf_end() {return mpChilds.end();}
    bf_const_iterator bf_end() const {return mpChilds.end();}

    /// Iterators to the supporting cell of this basis function
    cell_iterator cell_begin() {return mpCells.begin();}
    cell_const_iterator cell_begin() const {return mpCells.begin();}
    cell_iterator cell_end() {return mpCells.end();}
    cell_const_iterator cell_end() const {return mpCells.end();}

    /// Get the Id of this basis function. Each basis function should have unique Id
    const std::size_t& Id() const {return mId;}

    /// Set the Id for this basis function. One shall use this function only in the enumeration process.
    void SetId(const std::size_t& Id) {mId = Id;}

    /// Get the level of this basis function
    const std::size_t& Level() const {return mLevel;}

    /// Set the information in each direction
    void SetInfo(const int& dim, const std::size_t& Order) {mOrders[dim] = Order;}

    /// Get the order in specific direction
    const std::size_t& Order(const int& dim) const {return mOrders[dim];}

    /// return the internal reference of the knot vectors; use it with care
    const std::vector<knot_t>& LocalKnots(const int& dim) const {return mpLocalKnots[dim];}

    /// Get the local knot vectors
    template<class ValuesContainerType>
    void LocalKnots(const int& dim, ValuesContainerType& rKnots) const
    {
        if(rKnots.size() != mpLocalKnots[dim].size())
            rKnots.resize(mpLocalKnots[dim].size());
        for(std::size_t i = 0; i < mpLocalKnots[dim].size(); ++i)
            rKnots[i] = mpLocalKnots[dim][i]->Value();
    }

    /// Get the bounding box (=support domain) of this basis function
    std::vector<double> GetBoundingBox() const
    {
        double Xmin = static_cast<double>(INT_MAX);
        double Xmax = -Xmin;
        double Ymin = Xmin;
        double Ymax = -Ymin;
        double Zmin = Xmin;
        double Zmax = -Zmin;
        for(cell_iterator it = cell_begin(); it != cell_end(); ++it)
        {
            if((*it)->LeftValue() < Xmin)
                Xmin = (*it)->LeftValue();
            if((*it)->RightValue() > Xmax)
                Xmax = (*it)->RightValue();
            if((*it)->DownValue() < Ymin)
                Ymin = (*it)->DownValue();
            if((*it)->UpValue() > Ymax)
                Ymax = (*it)->UpValue();
            if((*it)->BelowValue() < Zmin)
                Zmin = (*it)->BelowValue();
            if((*it)->AboveValue() > Zmax)
                Zmax = (*it)->AboveValue();
        }

        if (TDim == 1)
            return std::vector<double>{Xmin, Xmax};
        else if (TDim == 2)
            return std::vector<double>{Xmin, Xmax, Ymin, Ymax};
        else if (TDim == 3)
            return std::vector<double>{Xmin, Xmax, Ymin, Ymax, Zmin, Zmax};
    }

    /// Get the refined coefficient of a child
    double GetRefinedCoefficient(int ChildId) const
    {
        std::map<int, double>::const_iterator it = mRefinedCoefficients.find(ChildId);
        if(it != mRefinedCoefficients.end())
            return it->second;
        else
        {
            std::stringstream ss;
            ss << "The basis function " << ChildId << " is not the child of basis function " << Id();
            KRATOS_THROW_ERROR(std::logic_error, ss.str(), "")
        }
    }

    /// check if this bf contain this local knot vectors. Two bfs are the same if they have exactly the same local knot vector. The order of the basis function is implied.
    // TODO change function name
    bool Contain(const std::vector<std::vector<knot_t> >& rpKnots) const
    {
        for (int dim = 0; dim < TDim; ++dim)
        {
            if (mpLocalKnots[dim].size() != rpKnots[dim].size())
                return false;

            for(std::size_t i = 0; i < mpLocalKnots[dim].size(); ++i)
                if(mpLocalKnots[dim][i] != rpKnots[dim][i])
                    return false;
        }

        return true;
    }

    /**************************************************************************
                            CONTROL VALUES
    **************************************************************************/

    template<class TVariableType>
    typename TVariableType::Type& GetValue(const TVariableType& rThisVariable)
    {
        return mData.GetValue(rThisVariable);
    }

    template<class TVariableType>
    const typename TVariableType::Type& GetValue(const TVariableType& rThisVariable) const
    {
        return mData.GetValue(rThisVariable);
    }

    template<class TVariableType>
    void SetValue(const TVariableType& rThisVariable, typename TVariableType::Type const& rValue)
    {
        mData.SetValue(rThisVariable, rValue);
    }

    template<class TDataType>
    bool Has(const Variable<TDataType>& rThisVariable) const
    {
        return mData.Has(rThisVariable);
    }

    template<class TAdaptorType>
    bool Has(const VariableComponent<TAdaptorType>& rThisVariable) const
    {
        return mData.Has(rThisVariable);
    }

    /**************************************************************************
                            COMPUTATION SUBROUTINES
    **************************************************************************/

    /// Compute the Bezier extraction operator of this basis function on the cell
    void ComputeExtractionOperator(Vector& Crow, const_cell_t p_cell)
    {
        std::vector<std::vector<double> > LocalKnots(TDim);
        std::vector<std::size_t> orders(TDim);
        for (int dim = 0; dim < TDim; ++dim)
        {
            orders[dim] = this->Order(dim);
            this->LocalKnots(dim, LocalKnots[dim]);
        }

        HBSplinesBasisFunction_Helper<TDim>::ComputeExtractionOperator(Crow, orders, LocalKnots, *p_cell);
    }

    /**************************************************************************
                            COMPARISON SUBROUTINES
    **************************************************************************/

    /// Implement relational operator for automatic arrangement in container
    inline bool operator==(const HBSplinesBasisFunction& rA) const
    {
        return this->Id() == rA.Id();
    }

    inline bool operator<(const HBSplinesBasisFunction& rA) const
    {
        return this->Id() < rA.Id();
    }

    /**************************************************************************
                            INFORMATION SUBROUTINES
    **************************************************************************/

    /// Print information of this basis function
    void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "Bf(id:" << Id() << "), p = (";
        for (int dim = 0; dim < TDim; ++dim)
            rOStream << " " << this->Order(dim);
        rOStream << ")";
    }

    /// Print data of this basis function
    void PrintData(std::ostream& rOStream) const
    {
        // Print the local knot vectors
        rOStream << " Local knot vectors:\n";
        for (int dim = 0; dim < TDim; ++dim)
        {
            rOStream << "  " << dim+1 << ":";
            for(std::size_t i = 0; i < mpLocalKnots[dim].size(); ++i)
                rOStream << " " << mpLocalKnots[dim][i]->Value();
            rOStream << std::endl;
        }

        // Print the cells
        rOStream << " Supporting cells:";
        std::size_t cnt = 0;
        for(cell_const_iterator it = cell_begin(); it != cell_end(); ++it)
            rOStream << std::endl << "  " << ++cnt << ": " << *(*it);
        if(cell_end() == cell_begin())
            rOStream << " none";
        rOStream << std::endl;
        rOStream << "List of children:";
        cnt = 0;
        for(bf_const_iterator it = bf_begin(); it != bf_end(); ++it)
        {
            std::map<int, double>::const_iterator it_coeff = mRefinedCoefficients.find((*it)->Id());
            rOStream << "  " << ++cnt << ": (" << (*it)->Id() << "," << it_coeff->second << ")";
        }
        if(bf_end() == bf_begin())
            rOStream << " none";
        rOStream << std::endl;
    }

private:

    std::size_t mId;
    std::size_t mLevel;
    boost::array<std::size_t, TDim> mOrders;
    bf_container_t mpChilds; // list of refined basis functions that constitute this basis function
    std::map<int, double> mRefinedCoefficients; // store the coefficient of refined basis functions
    cell_container_t mpCells; // list of cells support this basis function at the level of this basis function
    boost::array<std::vector<knot_t>, TDim> mpLocalKnots;

    /** A pointer to data related to this basis function. */
    DataValueContainer mData;
};

/// output stream function
template<int TDim>
inline std::ostream& operator <<(std::ostream& rOStream, const HBSplinesBasisFunction<TDim>& rThis)
{
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    return rOStream;
}

}// namespace Kratos.

#include "hbsplines_basis_function.hpp"

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_HBSPLINES_BASIS_FUNCTION_H_INCLUDED

