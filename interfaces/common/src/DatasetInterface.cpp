////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     DatasetInterface.cpp (interfaces)
//  Authors:  Chris Lovett
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "DatasetInterface.h"
#include "DatasetInterfaceImpl.h"

#include <common/include/DataLoaders.h>

#include <utilities/include/Exception.h>
#include <utilities/include/Files.h>

#include <vector>

using namespace ell;

namespace ELL_API
{

AutoDataVector::AutoDataVector() :
    _impl(std::make_shared<AutoDataVectorImpl>())
{
}

AutoDataVector::AutoDataVector(const std::vector<double>& data) :
    _impl(std::make_shared<AutoDataVectorImpl>())
{
    _impl->_vector = std::make_shared<ell::data::AutoDataVector>(data);
}

std::vector<double> AutoDataVector::ToArray() const
{
    return _impl->_vector->ToArray();
}

template <typename ElementType>
void InternalCopyTo(std::vector<double>& data, std::vector<ElementType>& buffer)
{
    if (buffer.size() < data.size())
    {
        buffer.resize(data.size()); // buffer must be at least this big
    }
    ::memset(buffer.data(), 0, buffer.size() * sizeof(ElementType));
    std::copy(data.begin(), data.end(), buffer.begin());
}

void AutoDataVector::CopyTo(std::vector<double>& buffer)
{
    // The problem we are solving here is that AutoDataVector could be a sparse vector which
    // has a random size depending on how many trailing zeros there are, so for AutoDataVector
    // to be useful in a Predict() call those trailing zeros have to be provided.  One can do
    // that using CopyTo where the incoming buffer has been sized to the right size for Predict
    // and we want to copy as many values as the AutoDataVector into the beginning of that buffer.
    auto source = _impl->_vector->ToArray();
    InternalCopyTo<double>(source, buffer);
}

void AutoDataVector::CopyTo(std::vector<float>& buffer)
{
    auto source = _impl->_vector->ToArray();
    InternalCopyTo<float>(source, buffer);
}

class AutoSupervisedExample::AutoSupervisedExampleImpl
{
public:
    AutoSupervisedExampleImpl() = default;
    AutoSupervisedExampleImpl(const ell::data::AutoSupervisedExample& e) :
        _example(e) {}
    ell::data::AutoSupervisedExample _example;
};

AutoSupervisedExample::AutoSupervisedExample() :
    _impl(std::make_shared<AutoSupervisedExampleImpl>())
{
}

AutoSupervisedExample::AutoSupervisedExample(AutoDataVector vector, double label) :
    _impl(std::make_shared<AutoSupervisedExampleImpl>(ell::data::AutoSupervisedExample(vector.ToArray(), { 1.0, label })))
{
}

double AutoSupervisedExample::GetLabel() const
{
    return _impl->_example.GetMetadata().label;
}

AutoDataVector AutoSupervisedExample::GetData() const
{
    auto result = AutoDataVector();
    result._impl->_vector = _impl->_example.GetSharedDataVector();
    return result;
}

AutoSupervisedDataset::AutoSupervisedDataset() :
    _impl(std::make_shared<AutoSupervisedDatasetImpl>())
{
}

size_t AutoSupervisedDataset::NumExamples() const
{
    return _impl->_dataset.NumExamples();
}

size_t AutoSupervisedDataset::NumFeatures() const
{
    return _impl->_dataset.NumFeatures();
}

AutoSupervisedExample AutoSupervisedDataset::GetExample(size_t index)
{
    AutoSupervisedExample result;
    result._impl->_example = _impl->_dataset.GetExample(index).CopyAs<ell::data::AutoSupervisedExample>();
    return result;
}

void AutoSupervisedDataset::Load(std::string filename)
{
    auto stream = utilities::OpenIfstream(filename);
    data::AutoSupervisedExampleIterator exampleIterator = ell::common::GetAutoSupervisedExampleIterator(stream);

    // load the dataset
    while (exampleIterator.IsValid())
    {
        auto example = exampleIterator.Get();
        auto mappedExample = data::AutoSupervisedExample(example.GetDataVector().ToArray(), example.GetMetadata());
        _impl->_dataset.AddExample(mappedExample);
        exampleIterator.Next();
    }
}

void AutoSupervisedDataset::AddExample(const AutoSupervisedExample& e)
{
    ell::data::AutoSupervisedExample ex(e.GetData().ToArray(), { 1.0, e.GetLabel() });
    _impl->_dataset.AddExample(ex);
}

void AutoSupervisedDataset::Save(std::string filename)
{
    auto stream = utilities::OpenOfstream(filename);
    _impl->_dataset.Print(stream);
}

} // namespace ELL_API
