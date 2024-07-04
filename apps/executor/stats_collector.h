#pragma once

#include <boost/accumulators/framework/accumulator_set.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/max.hpp>

#include <concepts>
#include <iostream>

namespace yb::task {

template <std::integral I>
class StatsCollector
{
   public:
    void collect(const I value) noexcept
    {
        cumulative_(value);
    }

    void print()
    {
        std::cout << "MEAN (ns): " << boost::accumulators::mean(cumulative_) << std::endl;
        std::cout << "MAX (ns): " << boost::accumulators::max(cumulative_) << std::endl;
    }

   private:
    boost::accumulators::accumulator_set<
        I, boost::accumulators::stats<boost::accumulators::tag::mean, boost::accumulators::tag::max> >
        cumulative_{};
};

}  // namespace yb::task