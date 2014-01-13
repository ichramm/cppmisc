/*!
 * \file atomic.hpp
 * \author ichramm
 *
 * Created on December 28, 2012, 09:50 PM
 */
#ifndef ichramm_utils_atomic_hpp__
#define ichramm_utils_atomic_hpp__

#include <boost/detail/atomic_count.hpp>

namespace ichramm
{
	namespace utils
	{
		typedef boost::detail::atomic_count  atomic_counter;
	}
}

#endif // ichramm_utils_atomic_hpp__
