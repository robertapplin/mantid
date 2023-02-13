// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"
#include <string>

using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::Kernel {

namespace {
/// static Logger definition
Logger g_log("FilteredTimeSeriesProperty");
} // namespace

/**
 * Constructor
 *  @param name :: The name to assign to the property
 */
template <typename TYPE>
FilteredTimeSeriesProperty<TYPE>::FilteredTimeSeriesProperty(const std::string &name)
    : TimeSeriesProperty<TYPE>(name), m_filter(std::make_unique<TimeROI>()), m_filterMap(), m_filterApplied(false) {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A pointer to a property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp,
                                                                 const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_filter(std::make_unique<TimeROI>()), m_filterApplied(false) {
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(const std::string &name,
                                                                 const std::vector<Types::Core::DateAndTime> &times,
                                                                 const std::vector<HeldType> &values)
    : TimeSeriesProperty<HeldType>(name, times, values), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterApplied(false) {}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterApplied(false) {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A smart pointer to take ownership of pointer to a
 * property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(
    std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp, const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp.get()), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterApplied(false) {
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

/**
 * "Virtual" copy constructor
 */
template <typename HeldType> FilteredTimeSeriesProperty<HeldType> *FilteredTimeSeriesProperty<HeldType>::clone() const {
  return new FilteredTimeSeriesProperty<HeldType>(*this);
}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(const FilteredTimeSeriesProperty &prop)
    : TimeSeriesProperty<HeldType>(prop.name(), prop.timesAsVector(), prop.valuesAsVector()),
      m_filter(std::make_unique<TimeROI>(*prop.m_filter.get())), m_filterMap(prop.m_filterMap),
      m_filterApplied(prop.m_filterApplied) {}

/**
 * Destructor
 */
template <typename HeldType> FilteredTimeSeriesProperty<HeldType>::~FilteredTimeSeriesProperty() = default;

/**
 * Get a vector of values taking the filter into account.
 * Values will be excluded if their times lie in a region where the filter is
 * false.
 * @returns :: Vector of included values only
 */
template <typename TYPE> std::vector<TYPE> FilteredTimeSeriesProperty<TYPE>::filteredValuesAsVector() const {
  if (this->m_filter->empty()) {
    return this->valuesAsVector(); // no filtering to do
  }
  if (!this->m_filterApplied) {
    applyFilter();
  }
  this->sortIfNecessary();

  std::vector<TYPE> filteredValues;
  for (const auto &value : this->m_values) {
    if (isTimeFiltered(value.time())) {
      filteredValues.emplace_back(value.value());
    }
  }

  return filteredValues;
}

/**
 * Return the time series's filtered times as a vector<DateAndTime>
 * @return A vector of DateAndTime objects
 */
template <typename HeldType>
std::vector<DateAndTime> FilteredTimeSeriesProperty<HeldType>::filteredTimesAsVector() const {
  if (m_filter->empty()) {
    return this->timesAsVector(); // no filtering to do
  }
  if (!m_filterApplied) {
    applyFilter();
  }
  this->sortIfNecessary();

  std::vector<DateAndTime> out;

  for (const auto &value : this->m_values) {
    if (isTimeFiltered(value.time())) {
      out.emplace_back(value.time());
    }
  }

  return out;
}

template <typename TYPE> double FilteredTimeSeriesProperty<TYPE>::mean() const {
  Mantid::Kernel::Statistics raw_stats =
      Mantid::Kernel::getStatistics(this->filteredValuesAsVector(), StatOptions::Mean);
  return raw_stats.mean;
}

/** Returns n-th valid time interval, in a very inefficient way.
 *
 * Here are some special cases
 *  (1) If empty property, throw runtime_error
 *  (2) If double or more entries, skip!
 *  (3) If n = size of property, use dt from last interval
 *  (4) If n > size of property, return Interval = 0
 *  @param n :: index
 *  @return n-th time interval
 */
template <typename TYPE> TimeInterval FilteredTimeSeriesProperty<TYPE>::nthInterval(int n) const {
  // Throw exception when there are no values
  if (this->m_values.empty()) {
    const std::string error("nthInterval(): FilteredTimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // Calculate time interval
  Kernel::TimeInterval deltaT;
  if (m_filter->empty()) {
    // No filter uses the parent class implmentation
    deltaT = TimeSeriesProperty<TYPE>::nthInterval(n);
  } else {
    this->applyFilter();

    // II. Filter
    // this intentionally ignores requests past the end because default deltaT value
    // has it covered
    if (static_cast<size_t>(n) < m_filterMap.size()) {
      // convert to a size_t to make life easier
      const size_t n_index = static_cast<size_t>(n);
      // create value for start time
      DateAndTime startTime = m_filter->getEffectiveTime(this->m_values[m_filterMap[n_index]].time());
      // create value for stop time
      DateAndTime stopTime;
      if (n_index + 1 < m_filterMap.size()) {
        // start with the "natural" stop time
        stopTime = this->m_values[m_filterMap[n_index + 1]].time();
      } else if (m_filter->empty()) {
        // this is a modified copy from TimeSeriesProperty

        // the last time is the last thing known
        const auto ultimate = this->m_values[m_filterMap.back()].time();
        // go backwards from the time before it that is different
        int counter = 0;
        while (DateAndTime::secondsFromDuration(ultimate -
                                                (this->m_values[*(m_filterMap.rbegin() + counter)].time())) == 0.) {
          counter += 1;
        }
        // get the last time that is different
        const auto penultimate = this->m_values[*(m_filterMap.rbegin() + counter)].time();
        auto lastDuration = ultimate - penultimate;
        // the last duration is equal to the previous, non-zero, duration
        stopTime = ultimate + lastDuration;
      } else {
        // TimeROI exists, just give the end of it
        stopTime = m_filter->lastTime();
      }

      // adjust the stop time according to the duration from the filter
      auto duration = m_filter->durationInSeconds(startTime, stopTime);
      // create a fake stop time using the duration
      stopTime = startTime + duration;

      // set the output value
      deltaT = TimeInterval(startTime, stopTime);
    }
  }

  return deltaT;
}

//-----------------------------------------------------------------------------------------------
/** Returns n-th value of n-th interval in an incredibly inefficient way.
 *  The algorithm is migrated from mthInterval()
 *  @param n :: index
 *  @return Value
 */
template <typename TYPE> TYPE FilteredTimeSeriesProperty<TYPE>::nthValue(int n) const {
  // Throw error if property is empty
  if (this->m_values.empty()) {
    const std::string error("nthValue(): FilteredTimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  TYPE value;
  if (m_filter->empty()) {
    // 3. Situation 1:  No filter
    value = TimeSeriesProperty<TYPE>::nthValue(n);
  } else {
    // 4. Situation 2: There is filter
    this->applyFilter();

    if (m_filterMap.empty()) {
      // this shouldn't happen
      value = this->m_values.back().value();
    } else {
      // going past the end gets the last value
      const size_t n_index = std::min<size_t>(static_cast<size_t>(n), m_filterMap.size() - 1);
      value = this->m_values[m_filterMap[n_index]].value();
    }
  }

  return value;
}

/* Divide the property into  allowed and disallowed time intervals according to
 \a filter.
 * (Repeated time-value pairs (two same time and value entries) mark the start
 of a gap in the values.)
 * If any time-value pair is repeated, it means that this entry is in disallowed
 region.
 * The gap ends and an allowed time interval starts when a single time-value is
 met.
   The disallowed region will be hidden for countSize() and nthInterval()
   Boundary condition
   ?. If filter[0].time > log[0].time, then all log before filter[0] are
 considered TRUE
   2. If filter[-1].time < log[-1].time, then all log after filter[-1] will be
 considered same as filter[-1]

   @param filter :: The filter mask to apply
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::filterWith(const TimeSeriesProperty<bool> *filter) {
  // reset the flag that this is filtered
  m_filterApplied = false;
  m_filterMap.clear();

  if ((!filter) || (filter->size() == 0)) {
    // if filter is empty, clear the current
    m_filter->clear();
  } else {
    m_filter->replaceROI(filter);

    applyFilter();
  }
}

/**
 * Restores the property to the unsorted & unfiltered state
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::clearFilter() {
  m_filter->clear();
  m_filterMap.clear();
}

/**
 * Updates size()
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::countSize() const {
  if (m_filter->empty()) {
    // 1. Not filter
    this->m_size = int(this->m_values.size());
  } else {
    // 2. With Filter
    this->applyFilter();
    this->m_size = int(m_filterMap.empty() ? this->m_values.size() : m_filterMap.size());
  }
}

/*
 * Apply filter
 * Requirement: There is no 2 consecutive 'second' values that are same in
 *mFilter
 *
 * It only works with filter starting from TRUE AND having TRUE and FALSE
 *altered
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::applyFilter() const {
  // 1. Check and reset
  if (m_filterApplied)
    return;

  // clear out the previous version
  m_filterMap.clear();

  if (m_filter->empty())
    return;

  // 2. Apply filter
  this->sortIfNecessary();

  // the index into the m_values array of the time, or -1 (before) or m_values.size() (after)
  std::size_t index_current_log{0};

  for (const auto splitter : m_filter->toSplitters()) {
    const auto endTime = splitter.end();

    // check if the splitter starts too early
    if (endTime < this->m_values[index_current_log].time()) {
      continue; // skip to the next splitter
    }

    // cache values to reduce number of method calls
    const auto beginTime = splitter.begin();

    // find the first log that should be added
    while (this->m_values[index_current_log].time() <= beginTime) {
      index_current_log++;
    }
    // need to back up by one
    if (index_current_log > 0)
      index_current_log--;

    // if the first value to be added is already in the filter-map, remove it now
    // to make the logic in the loop simpler
    if ((!m_filterMap.empty()) && (index_current_log == m_filterMap.back()))
      m_filterMap.pop_back();

    // add everything up to the end time
    for (; index_current_log < this->m_values.size(); ++index_current_log) {
      if (this->m_values[index_current_log].time() >= endTime)
        break;
      m_filterMap.emplace_back(index_current_log);
    }
  }

  // Change flag
  m_filterApplied = true;

  // Re-count size
  countSize();
}

/**
 * Set the value of the property via a reference to another property.
 * If the value is unacceptable the value is not changed but a string is
 * returned.
 * The value is only accepted if the other property has the same type as this
 * @param right :: A reference to a property.
 */
template <typename TYPE> std::string FilteredTimeSeriesProperty<TYPE>::setValueFromProperty(const Property &right) {
  auto prop = dynamic_cast<const FilteredTimeSeriesProperty<TYPE> *>(&right);
  if (!prop) {
    return "Could not set value: properties have different type.";
  }
  this->m_values = prop->m_values;
  this->m_size = prop->m_size;
  this->m_propSortedFlag = prop->m_propSortedFlag;
  m_filter = std::unique_ptr<TimeROI>(prop->m_filter.get());
  m_filterMap = prop->m_filterMap;
  m_filterApplied = prop->m_filterApplied;
  return "";
}

/**
 * Combines the currently held filter with the supplied one as an intersection.
 */
template <typename TYPE>
Kernel::TimeROI *FilteredTimeSeriesProperty<TYPE>::unionFilterWithOther(const TimeROI *other) const {
  auto roi = new TimeROI(*m_filter.get());
  if (other && !other->empty()) // find intersection between the internal ROI and
                                // the supplied ROI
    roi->update_intersection(*other);
  return roi;
}

/**
 * Find out if the given time is included in the filtered data
 * i.e. it does not lie in an excluded region. This function assumes
 * the filter is not empty, it has been applied and the values are
 * sorted by time.
 * @param time :: [input] Time to check
 * @returns :: True if time is in an included region, false if the filter
 * excludes it.
 */
template <typename TYPE>
bool FilteredTimeSeriesProperty<TYPE>::isTimeFiltered(const Types::Core::DateAndTime &time) const {
  // Each time/value pair in the filter defines a point where the region defined
  // after that time is either included/excluded depending on the boolean value.
  // By definition of the filter construction the region before a given filter
  // time must have the opposite value. For times outside the filter region:
  //   1. time < first filter time: inverse of the first filter value
  //   2. time > last filter time: value of the last filter value
  // If time == a filter time then the value is taken to belong to that filter
  // region and not the previous
  if (m_filter->empty()) {
    return true;
  } else {
    return m_filter->valueAtTime(time);
  }
}

/**
 * Get a list of the splitting intervals, if filtering is enabled.
 * Otherwise the interval is just first time - last time.
 * @returns :: Vector of splitting intervals
 */
template <typename TYPE>
std::vector<SplittingInterval> FilteredTimeSeriesProperty<TYPE>::getSplittingIntervals() const {
  if (m_filter->empty()) {
    // Case where there is no filter
    std::vector<SplittingInterval> intervals;
    intervals.emplace_back(this->firstTime(), this->lastTime());
    return intervals;
  } else {
    if (!m_filterApplied) {
      applyFilter();
    }

    return m_filter->toSplitters();
  }
}

/** Returns the calculated time weighted average value.
 * @param timeRoi  Object that holds information about when the time measurement
 * was active.
 * @return The time-weighted average value of the log when the time measurement
 * was active.
 */
template <typename TYPE> double FilteredTimeSeriesProperty<TYPE>::timeAverageValue(const TimeROI *timeRoi) const {
  // make a copy of the filter
  auto internalRoi = this->unionFilterWithOther(timeRoi);
  // call parent method
  return TimeSeriesProperty<TYPE>::timeAverageValue(internalRoi);
}

/**
 * Return a TimeSeriesPropertyStatistics struct containing the
 * statistics of this TimeSeriesProperty object.
 * @param roi : Optional TimeROI pointer to get statistics for active time.
 *
 * N.B. This method DOES take filtering into account
 */
template <typename TYPE>
TimeSeriesPropertyStatistics FilteredTimeSeriesProperty<TYPE>::getStatistics(const TimeROI *roi) const {
  // make a copy of the filter
  auto internalRoi = this->unionFilterWithOther(roi);
  // call parent method
  return TimeSeriesProperty<TYPE>::getStatistics(internalRoi);
}

/** Calculate a particular statistical quantity from the values of the time series.
 *  @param selection : Enum indicating the selected statistical quantity.
 *  @param roi : optional pointer to TimeROI object for filtering the time series values.
 *  @return The value of the computed statistical quantity.
 */
template <typename TYPE>
double FilteredTimeSeriesProperty<TYPE>::extractStatistic(Math::StatisticType selection, const TimeROI *roi) const {
  // make a copy of the filter
  auto internalRoi = this->unionFilterWithOther(roi);
  // call parent method
  return TimeSeriesProperty<TYPE>::extractStatistic(selection, internalRoi); // call parent method
}

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE) template class MANTID_KERNEL_DLL FilteredTimeSeriesProperty<TYPE>;

// -------------------------- Concrete instantiation
// -----------------------------------------------
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(std::string)
INSTANTIATE(bool)

///@endcond

} // namespace Mantid::Kernel
