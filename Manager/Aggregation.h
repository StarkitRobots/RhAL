#pragma once

namespace RhAL {

/**
 * All value Aggregation policy
 */
enum AggregationPolicy {
    //Keep the last written value
    AggregateLast,
    //Keep the first written value
    AggregateFirst,
    //Sum all values
    AggregateSum,
    //Keep the maximum value
    AggregateMax,
    //Keep the minimum value
    AggregateMin,
};

/**
 * Policies implementation
 */
namespace Impl {
    template <typename T>
    T implAggregateLast(T oldValue, T newValue)
    {
        (void)oldValue;
        return newValue;
    }
    template <typename T>
    T implAggregateFirst(T oldValue, T newValue)
    {
        (void)newValue;
        return oldValue;
    }
    template <typename T>
    T implAggregateSum(T oldValue, T newValue)
    {
        return oldValue + newValue;
    }
    template <typename T>
    T implAggregateMax(T oldValue, T newValue)
    {
        return (oldValue > newValue) ? oldValue : newValue;
    }
    template <typename T>
    T implAggregateMin(T oldValue, T newValue)
    {
        return (oldValue < newValue) ? oldValue : newValue;
    }
    /**
     * Specialization for boolean
     */
    template <>
    bool implAggregateSum(bool oldValue, bool newValue)
    {
        return oldValue || newValue;
    }
    template <>
    bool implAggregateMax(bool oldValue, bool newValue)
    {
        return oldValue || newValue;
    }
    template <>
    bool implAggregateMin(bool oldValue, bool newValue)
    {
        return oldValue && newValue;
    }
}

/**
 * Return the aggregated value for given policy 
 * with given old aggregated value and 
 * new written value
 */
template <typename T>
T aggregateValue(AggregationPolicy policy, 
    T oldValue, T newValue)
{
    switch (policy) {
        case AggregateLast: 
            return Impl::implAggregateLast(oldValue, newValue); 
            break;
        case AggregateFirst: 
            return Impl::implAggregateFirst(oldValue, newValue); 
            break;
        case AggregateSum: 
            return Impl::implAggregateSum(oldValue, newValue); 
            break;
        case AggregateMax: 
            return Impl::implAggregateMax(oldValue, newValue); 
            break;
        case AggregateMin: 
            return Impl::implAggregateMin(oldValue, newValue); 
            break;
        default: 
            return T();
            break;
    }
}

}
