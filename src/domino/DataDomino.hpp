/**
 * Copyright 2020 Nokia. All rights reserved.
 */
// ***********************************************************************************************
// - what:  Domino to store any type of data (no read/write protect)
// - VALUE: eg C&M translator store any type data from RU/BBU/HLAPI, like IM [MUST-HAVE!]
// - core:  dataStore_
// - scope: provide min interface (& extend by non-member func)
// ***********************************************************************************************
#pragma once

#include <unordered_map>
#include <memory>  // make_shared

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class DataDomino : public aDominoType
{
    // -------------------------------------------------------------------------------------------
    // Extend Tile record:
    // - data: now Tile can store any type of data besides state, optional
    // -------------------------------------------------------------------------------------------

public:
    // -------------------------------------------------------------------------------------------
    // - for read/write data
    // - if aEvName/data invalid, return null
    // - not template<aDataType> so can virtual for WrDatDom
    // . & let DataDomino has little idea of read-write ctrl, simpler
    virtual std::shared_ptr<void> getShared(const Domino::EvName& aEvName);

    size_t nShared(const Domino::EvName&) const;

    // -------------------------------------------------------------------------------------------
    // - replace old data by new=aSharedData if old != new
    // - for aDataType w/o default constructor!!!
    virtual void replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData);

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, std::shared_ptr<void> > dataStore_;  // [event]=shared_ptr<"DataType">
public:
    using aDominoType::log_;
};

// ***********************************************************************************************
template<typename aDominoType>
std::shared_ptr<void> DataDomino<aDominoType>::getShared(const Domino::EvName& aEvName)
{
    const auto ev = this->newEvent(aEvName);
    auto&& data = dataStore_[ev];
    return (data.use_count() > 0) ? data : std::shared_ptr<void>();
}

// ***********************************************************************************************
template<typename aDominoType>
size_t DataDomino<aDominoType>::nShared(const Domino::EvName& aEvName) const
{
    auto&& found = dataStore_.find(this->getEventBy(aEvName));
    return (found == dataStore_.end()) ? 0 : found->second.use_count();
}

// ***********************************************************************************************
template<typename aDominoType>
void DataDomino<aDominoType>::replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData)
{
    dataStore_[this->newEvent(aEvName)] = aSharedData;
}



#define EXTEND_INTERFACE_FOR_DATA_DOMINO  // more friendly than min DataDomino interface
// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
aDataType getValue(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    auto&& data = std::static_pointer_cast<aDataType>(aDom.getShared(aEvName));
    if (data.use_count() > 0) return *data;

    auto&& log_ = aDom.log_;
    WRN("(DataDomino) Failed!!! EvName=" << aEvName << " not found, return undefined obj!!!");
    return aDataType();
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
void setValue(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData)
{
    auto&& data = std::make_shared<aDataType>(aData);
    aDom.replaceShared(aEvName, data);
}
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-07-27  CSZ       1)create
// 2021-04-01  CSZ       - coding req
// 2021-04-30  ZQ & CSZ  2)ZhangQiang told CSZ shared_ptr<void> can del Data correctly
// 2021-09-13  CSZ       - new interface to read/write value directly
// 2021-09-14  CSZ       3)writable ctrl eg for Yang rw & ro para
// 2021-12-31  PJ & CSZ  - formal log & naming
// ***********************************************************************************************
