/*!
 * \file   synchronizer.hpp
 * \author ichramm
 *
 * Created on January 14, 2013, 9:15 PM
 */
#ifndef ichramm_utils_synchronizer_hpp__
#define ichramm_utils_synchronizer_hpp__

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace ichramm
{
	namespace utils
	{
		/*!
		 * This class works like a manual-reset event.
		 *
		 * It helps to synchronize two or more threads when a thread
		 * has to met certain condition and the other(s) have to
		 * wait until the condition is met.
		 *
		 * \note Wait functions are designed to cope with spurious
		 * wakeups so the user does not have to worry about checking
		 * whether the condition was actually set or not.
		 */
		class synchronizer
			: private boost::noncopyable
		{
		public:

			/*!
			 * Lockable concept
			 */
			typedef boost::unique_lock<synchronizer> scoped_lock;

			/*!
			 * Creates a new \c synchronizer object with condition set to \c false
			 */
			synchronizer()
			 : _condition_met(false)
			{ }

			/*!
			 * \return \c true if the condition is met
			 *
			 * \remarks This function acquires the lock
			 */
			bool is_condition_met() const
			{
				boost::lock_guard<boost::mutex> lock(_sync_mutex);
				return _condition_met;
			}

			/*!
			 * \return \c true if the condition is met (non-const version)
			 *
			 * It really does not make much sense to call this function but
			 * if you want it then go on...
			 *
			 * \remarks The lock must be held by \p lock
			 */
			bool is_condition_met(scoped_lock& lock)
			{
				if (lock.owns_lock() == false)
				{
					throw std::runtime_error("Invalid lock");
				}

				return _condition_met;
			}

			/*!
			 * Sets the condition to true and notifies all waiters of the change
			 *
			 * \note This function acquires the lock
			 */
			void set()
			{
				scoped_lock lock(*this);
				set(lock);
			}

			/*!
			 * Sets the condition to true and notifies all waiters of the change
			 *
			 * \remarks The lock must be held by \p lock
			 */
			void set(scoped_lock& lock)
			{
				if (lock.owns_lock() == false)
				{
					throw std::runtime_error("Invalid lock");
				}

				_condition_met = true;
				_condition.notify_all();
			}

			/*!
			 * Resets the condition to \c false
			 *
			 * \note This function acquires the lock
			 */
			void reset()
			{
				scoped_lock lock(*this);
				reset(lock);
			}

			/*!
			 * Resets the condition to \c false
			 *
			 * \remarks The lock must be held by \p lock
			 */
			void reset(scoped_lock& lock)
			{
				if (lock.owns_lock() == false)
				{
					throw std::runtime_error("Invalid lock");
				}

				_condition_met = false;
			}

			/*!
			 * Acquires the lock (BasicLockable concept)
			 *
			 * \see http://www.boost.org/doc/html/thread/synchronization.html#thread.synchronization.mutex_concepts.lockable
			 */
			void lock()
			{
				_sync_mutex.lock();
			}

			/*!
			 * Releases the lock (BasicLockable concept)
			 *
			 * \see http://www.boost.org/doc/html/thread/synchronization.html#thread.synchronization.mutex_concepts.lockable
			 */
			void unlock()
			{
				_sync_mutex.unlock();
			}

			/*!
			 * Tries to acquire the lock (Lockable concept)
			 *
			 * return \c true on success
			 *
			 * \see http://www.boost.org/doc/html/thread/synchronization.html#thread.synchronization.mutex_concepts.lockable
			 */
			bool try_lock()
			{
				return _sync_mutex.try_lock();
			}

			/*!
			 * Waits until the condition is set
			 *
			 * \note This function acquires the lock
			 */
			void wait()
			{
				scoped_lock lock(*this);
				wait(lock);
			}

			/*!
			 * Waits until the condition is set using the specified lock
			 *
			 * \remarks The lock must be held by \p lock
			 */
			void wait(scoped_lock& lock)
			{
				if (lock.owns_lock() == false)
				{
					throw std::runtime_error("Invalid lock");
				}

				while (_condition_met == false)
				{ // cope with spurious wakeups
					_condition.wait(lock);
				}
			}

			/*!
			 * Waits until the condition is set or current time as reported
			 * by \c boost::get_system_time() is greater than or equal
			 * to \code boost::get_system_time() + timeout \endcode
			 *
			 * \note This function acquires the lock
			 */
			bool wait(const boost::posix_time::time_duration& timeout)
			{
				scoped_lock lock(*this);
				return wait(lock, timeout);
			}

			/*!
			 * Waits until the condition is set or current time as reported
			 * by \c boost::get_system_time() is greater than or equal
			 * to \code boost::get_system_time() + timeout \endcode
			 *
			 * \remarks The lock must be held by \p lock
			 */
			bool wait(scoped_lock& lock, const boost::posix_time::time_duration& timeout)
			{
				return wait(lock, boost::get_system_time() + timeout);
			}

			/*!
			 * Waits until the condition is set or current time as specified
			 * by \c boost::get_system_time() is greater than or equal to \p deadline
			 *
			 * \note This function acquires the lock
			 */
			bool wait(const boost::system_time& deadline)
			{
				scoped_lock lock(*this);
				return wait(lock, deadline);
			}

			/*!
			 * Waits until the condition is set or current time as specified
			 * by \c boost::get_system_time() is greater than or equal to \p deadline
			 *
			 * \remarks The lock must be held by \p lock
			 */
			bool wait(scoped_lock& lock, const boost::system_time& deadline)
			{
				if (lock.owns_lock() == false)
				{
					throw std::runtime_error("Invalid lock");
				}

				while (_condition_met == false && boost::get_system_time() < deadline)
				{ // cope with spurious wakeups
					_condition.timed_wait(lock, deadline);
				}

				return _condition_met;
			}

		private:
			bool                 _condition_met;
			boost::condition     _condition;
			mutable boost::mutex _sync_mutex;
		};
	}
}

#endif // ichramm_utils_synchronizer_hpp__

