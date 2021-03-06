//
//   Project Name:        Kratos
//   Last Modified by:    $Author: hbui $
//   Date:                $Date: 2 Dec 2017 $
//   Revision:            $Revision: 1.0 $
//
//

#if !defined(KRATOS_ISOGEOMETRIC_APPLICATION_WEIGHTED_WeightedFESpace_H_INCLUDED )
#define  KRATOS_ISOGEOMETRIC_APPLICATION_WEIGHTED_WeightedFESpace_H_INCLUDED

// System includes
#include <vector>

// External includes


// Project includes
#include "includes/define.h"
#include "includes/serializer.h"
#include "custom_utilities/fespace.h"


namespace Kratos
{

/**
 * A weighted FESpace add the weighted information to the FESpace.
 */
template<int TDim>
class WeightedFESpace : public FESpace<TDim>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(WeightedFESpace);

    /// Type definition
    typedef FESpace<TDim> BaseType;
    typedef typename BaseType::cell_container_t cell_container_t;

    /// Default constructor
    WeightedFESpace(typename BaseType::Pointer pFESpace, const std::vector<double>& weights)
    : BaseType(), mpFESpace(pFESpace), mWeights(weights) {}

    /// Destructor
    virtual ~WeightedFESpace()
    {
    }

    /// Helper to create new BSplinesWeightedFESpace pointer
    static WeightedFESpace<TDim>::Pointer Create(typename BaseType::Pointer pFESpace, const std::vector<double>& weights)
    {
        return WeightedFESpace<TDim>::Pointer(new WeightedFESpace(pFESpace, weights));
    }

    /// Get the number of basis functions defined over the WeightedFESpace
    virtual const std::size_t TotalNumber() const
    {
        return mpFESpace->TotalNumber();
    }

    /// Get the order of the WeightedFESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return mpFESpace->Order(i);
    }

    /// Set the weight vector
    void SetWeights(const std::vector<double>& Weights) {mWeights = Weights;}

    /// Get the weight vector
    const std::vector<double>& Weights() const {return mWeights;}

    /// Get the string representing the type of the WeightedFESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string representing the type of the WeightedFESpace
    static std::string StaticType()
    {
        std::stringstream ss;
        ss << "WeightedFESpace" << TDim << "D";
        return ss.str();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Get the values of the basis function i at point xi
    virtual double GetValue(const std::size_t& i, const std::vector<double>& xi) const
    {
        std::vector<double> values = mpFESpace->GetValue(xi);
        double sum_value = 0.0;
        for (std::size_t j = 0; j < values.size(); ++j)
            sum_value += mWeights[j] * values[j];
        return mWeights[i]*values[i] / sum_value;
    }

    /// Get the values of the basis functions at point xi
    virtual std::vector<double> GetValue(const std::vector<double>& xi) const
    {
        std::vector<double> values = mpFESpace->GetValue(xi);
        std::vector<double> new_values(values.size());
        double sum_value = 0.0;
        for (std::size_t i = 0; i < values.size(); ++i)
            sum_value += mWeights[i] * values[i];
        for (std::size_t i = 0; i < new_values.size(); ++i)
            new_values[i] = mWeights[i]*values[i] / sum_value;
        return new_values;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Enumerate the dofs of each grid function. The enumeration algorithm is pretty straightforward.
    /// If the dof does not have pre-existing value, which assume it is -1, it will be assigned the incremental value.
    virtual std::size_t& Enumerate(std::size_t& start)
    {
        return mpFESpace->Enumerate(start);
    }

    /// Check the compatibility between boundaries of two WeightedFESpacees
    virtual bool CheckBoundaryCompatibility(const FESpace<TDim>& rFESpace1, const BoundarySide& side1,
            const FESpace<TDim>& rFESpace2, const BoundarySide& side2) const
    {
        return rFESpace1 == rFESpace2;
    }

    /// Validate the WeightedFESpace before using
    virtual bool Validate() const
    {
        if (mWeights.size() != this->TotalNumber())
        {
            KRATOS_THROW_ERROR(std::logic_error, "The weight information is incorrect", "")
            return false;
        }
        return mpFESpace->Validate();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Extract the index of the functions on the boundary
    virtual std::vector<std::size_t> ExtractBoundaryFunctionIndices(const BoundarySide& side) const
    {
        return mpFESpace->ExtractBoundaryFunctionIndices(side);
    }

    /// Assign the index for the functions on the boundary
    virtual void AssignBoundaryFunctionIndices(const BoundarySide& side, const std::vector<std::size_t>& func_indices)
    {
        mpFESpace->AssignBoundaryFunctionIndices(side, func_indices);
    }

    /// Construct the boundary WeightedFESpace based on side
    virtual typename FESpace<TDim-1>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    {
        typename FESpace<TDim-1>::Pointer pBFESpace = mpFESpace->ConstructBoundaryFESpace(side);
        // TODO extract/compute the weights on the boundary
        KRATOS_THROW_ERROR(std::logic_error, __FUNCTION__, "is not completed")
        std::vector<double> boundary_weights;

        return typename WeightedFESpace<TDim-1>::Pointer(new WeightedFESpace<TDim-1>(pBFESpace, boundary_weights));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Compare the two weighted FESpaces in terms of its parametric information.
    virtual bool IsCompatible(const FESpace<TDim>& rOtherFESpace) const
    {
        if (rOtherFESpace.Type() == this->Type())
        {
            const WeightedFESpace<TDim>& rOtherWeightedFESpace = dynamic_cast<const WeightedFESpace<TDim>&>(rOtherFESpace);
            if (this->Weights().size() != rOtherWeightedFESpace.Weights().size())
            {
                return false;
            }
            else
            {
                for (std::size_t i = 0; i < this->Weights().size(); ++i)
                    if (this->Weights()[i] != rOtherWeightedFESpace.Weights()[i])
                        return false;
                return rOtherFESpace.IsCompatible(static_cast<const FESpace<TDim>&>(*this));
            }
        }
        else
        {
            for (std::size_t i = 0; i < this->Weights().size(); ++i)
                if (this->Weights()[i] != 1.0)
                    return false;
            return rOtherFESpace.IsCompatible(static_cast<const FESpace<TDim>&>(*this));
        }
        return false;
    }

    /// Overload comparison operator
    virtual bool operator==(const WeightedFESpace<TDim>& rOther) const
    {
        return this->IsCompatible(rOther);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Create the cell manager for all the cells in the support domain of the WeightedFESpace
    virtual typename cell_container_t::Pointer ConstructCellManager() const
    {
        typename cell_container_t::Pointer pCellManager = mpFESpace->ConstructCellManager();
        // TODO add weight information to the cells
        KRATOS_THROW_ERROR(std::logic_error, __FUNCTION__, "is not completed")
        return pCellManager;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Overload assignment operator
    WeightedFESpace<TDim>& operator=(const WeightedFESpace<TDim>& rOther)
    {
        BaseType::operator=(rOther);
        this->mpFESpace = rOther.mpFESpace;
        this->mWeights = rOther.mWeights;
        return *this;
    }

    /// Clone this WeightedFESpace, this is a deep copy operation
    virtual typename FESpace<TDim>::Pointer Clone() const
    {
        typename WeightedFESpace<TDim>::Pointer pNewWeightedFESpace = typename WeightedFESpace<TDim>::Pointer(new WeightedFESpace<TDim>(mpFESpace, mWeights));
        *pNewWeightedFESpace = *this;
        return pNewWeightedFESpace;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        BaseType::PrintInfo(rOStream);
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
        BaseType::PrintData(rOStream);
        rOStream << " Weights:";
        for (std::size_t i = 0; i < mWeights.size(); ++i)
            rOStream << " " << mWeights[i];
    }

private:

    typename BaseType::Pointer mpFESpace;
    std::vector<double> mWeights;

    /// Serializer
    friend class Serializer;

    virtual void save(Serializer& rSerializer) const
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS( rSerializer,  BaseType );
        rSerializer.save( "mWeights", mWeights );
    }

    virtual void load(Serializer& rSerializer)
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS( rSerializer,  BaseType );
        rSerializer.load( "mWeights", mWeights );
    }
};

/**
 * Template specific instantiation for null-D WeightedFESpace to terminate the compilation
 */
template<>
class WeightedFESpace<0> : public FESpace<0>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(WeightedFESpace);

    // Type definitions
    typedef CellManager<Cell> cell_container_t;

    /// Default constructor
    WeightedFESpace(FESpace<0>::Pointer pFESpace, const std::vector<double>& weights) : mFunctionId(-1) {}

    /// Destructor
    virtual ~WeightedFESpace() {}

    /// Get the number of basis functions defined over the WeightedFESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the WeightedFESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the WeightedFESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the WeightedFESpace
    static std::string StaticType()
    {
        return "FESpace0D";
    }

    /// Overload comparison operator
    virtual bool operator==(const WeightedFESpace<0>& rOther) const
    {
        return true;
    }

    /// Reset all the dof numbers for each grid function to -1
    void ResetFunctionIndices()
    {
        mFunctionId = -1;
    }

    /// Reset the function indices to a given values
    void ResetFunctionIndices(const std::vector<std::size_t>& func_indices)
    {
        assert(func_indices.size() == 1);
        mFunctionId = func_indices[0];
    }

    /// Get the vector of function indices
    std::vector<std::size_t> FunctionIndices() const {return std::vector<std::size_t>{mFunctionId};}

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const WeightedFESpace<0>& rFESpace1, const BoundarySide& side1,
            const WeightedFESpace<0>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the WeightedFESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary WeightedFESpace based on side
    // virtual typename WeightedFESpace<-1>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "WeightedFESpace<0>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }

protected:

    std::size_t mFunctionId;

private:

    /// Serializer
    friend class Serializer;

    virtual void save(Serializer& rSerializer) const
    {
    }

    virtual void load(Serializer& rSerializer)
    {
    }
};


/**
 * Template specific instantiation for -1-D WeightedFESpace to terminate the compilation
 */
template<>
class WeightedFESpace<-1> : public FESpace<-1>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(WeightedFESpace);

    /// Default constructor
    WeightedFESpace(FESpace<-1>::Pointer pFESpace, const std::vector<double>& weights) {}

    /// Destructor
    virtual ~WeightedFESpace() {}

    /// Get the number of basis functions defined over the WeightedFESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the WeightedFESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the WeightedFESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the WeightedFESpace
    static std::string StaticType()
    {
        return "WeightedFESpace<-1>D";
    }

    /// Overload comparison operator
    virtual bool operator==(const WeightedFESpace<-1>& rOther) const
    {
        return true;
    }

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const WeightedFESpace<-1>& rFESpace1, const BoundarySide& side1,
            const WeightedFESpace<-1>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the WeightedFESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary WeightedFESpace based on side
    // virtual typename WeightedFESpace<-2>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "WeightedFESpace<-1>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }
};


/**
 * Template specific instantiation for -2-D WeightedFESpace to terminate the compilation
 */
template<>
class WeightedFESpace<-2> : public FESpace<-2>
{
public:
    /// Pointer definition
    KRATOS_CLASS_POINTER_DEFINITION(WeightedFESpace);

    /// Default constructor
    WeightedFESpace(FESpace<-2>::Pointer pFESpace, const std::vector<double>& weights) {}

    /// Destructor
    virtual ~WeightedFESpace() {}

    /// Get the number of basis functions defined over the WeightedFESpace
    virtual const std::size_t TotalNumber() const
    {
        return 0;
    }

    /// Get the order of the WeightedFESpace in specific direction
    virtual const std::size_t Order(const std::size_t& i) const
    {
        return 0;
    }

    /// Get the string describing the type of the WeightedFESpace
    virtual std::string Type() const
    {
        return StaticType();
    }

    /// Get the string describing the type of the WeightedFESpace
    static std::string StaticType()
    {
        return "WeightedFESpace<-2>D";
    }

    /// Overload comparison operator
    virtual bool operator==(const WeightedFESpace<-2>& rOther) const
    {
        return true;
    }

    /// Check the compatibility between boundaries of two FESpacees
    virtual bool CheckBoundaryCompatibility(const WeightedFESpace<-2>& rFESpace1, const BoundarySide& side1,
            const WeightedFESpace<-2>& rFESpace2, const BoundarySide& side2) const
    {
        return true;
    }

    /// Validate the WeightedFESpace before using
    virtual bool Validate() const
    {
        return true;
    }

    // /// Construct the boundary WeightedFESpace based on side
    // virtual typename WeightedFESpace<-3>::Pointer ConstructBoundaryFESpace(const BoundarySide& side) const
    // {
    //     return NULL;
    // }

    /// Information
    virtual void PrintInfo(std::ostream& rOStream) const
    {
        rOStream << "WeightedFESpace<-2>";
    }

    virtual void PrintData(std::ostream& rOStream) const
    {
    }
};

/// output stream function
template<int TDim>
inline std::ostream& operator <<(std::ostream& rOStream, const WeightedFESpace<TDim>& rThis)
{
    rOStream << "-------------Begin WeightedFESpaceInfo-------------" << std::endl;
    rThis.PrintInfo(rOStream);
    rOStream << std::endl;
    rThis.PrintData(rOStream);
    rOStream << std::endl;
    rOStream << "-------------End WeightedFESpaceInfo-------------";
    return rOStream;
}

} // namespace Kratos.

#undef DEBUG_DESTROY

#endif // KRATOS_ISOGEOMETRIC_APPLICATION_WEIGHTED_FESPACE_H_INCLUDED defined

