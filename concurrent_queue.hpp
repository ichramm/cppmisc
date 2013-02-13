/*!
 * \file concurrent_queue.hpp
 * \author ichramm
 *
 * Created on August 3, 2010, 2:53 PM
 */
#ifndef ichramm_utils_concurrent_queue_hpp__
#define ichramm_utils_concurrent_queue_hpp__

#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "atomic.hpp"

namespace ichramm
{
	namespace utils
	{
		/*!
		 * This class implements FIFO queue, but with concurrency.
		 *
		 * This means not only that this object is thread safe, but also will
		 * block any \c pop() call when the queue is empty until a new element
		 * is pushed into the queue or, in some overloads, the given timeout
		 * has passed.
		 */
		template <
			class value_type,
			class container_type = std::list<value_type>
		> class concurrent_queue
		{
		public:

			/*!
			 * Thrown a wait in \c pop() has timed out
			 */
			class timeout_exception
				: public std::exception
			{
			public:

				/*!
				 * Overrides \c std::exception::what
				 */
				const char* what() const
				{
					return "Timed-out";
				}
			};

			/*!
			 * Initializes an empty queue
			 */
			concurrent_queue()
			 : _size(0)
			{
			}

			/*!
			 * \return \c true if the queue does not contain any elements, otherwise \c false
			 */
			bool empty() const
			{
				return (_size == 0);
			}

			/*!
			 * \return The current number of elements in the queue
			 */
			size_t size() const
			{
				return _size;
			}

			/*!
			 * Inserts an element at the end of the queue
			 *
			 * \note If there is a thread blocked in \c pop(), this function will wake it up
			 */
			void push(const value_type& element)
			{
				boost::lock_guard<boost::mutex> lock(_mutex);
				do_push(element);
			}

			/*!
			 * Gets and removes an element from the front of the queue. If the
			 * queue is empty this function blocks until a new element is pushed
			 * into the stack.
			 *
			 * This is the recommended popping method when used wisely.
			 *
			 * \return The first element in the queue, aka the one being popped
			 */
			value_type pop()
			{
				value_type _val;
				return pop(_val);
			}

			/*!
			 * Gets and removes an element from the front of the queue. If the
			 * queue is empty this function blocks until a new element is pushed
			 * into the stack, or until \p timeout_ms milliseconds has passed.
			 *
			 * \param timeout_ms Max milliseconds to wait in case the queue is empty
			 *
			 * \return The first element in the queue, aka the one being popped
			 *
			 * \throw timeout_exception
			 */
			value_type pop(size_t timeout_ms)
			{
				value_type _result;

				if ( !pop(_result, timeout_ms) )
				{
					throw timeout_exception();
				}

				return _result;
			}

			/*!
			 * Gets and removes an element from the front of the queue. If the
			 * queue is empty this function blocks until a new element is pushed
			 * into the stack.
			 *
			 * \param result Set with the element being popped.
			 *
			 * \return \p result
			 */
			value_type& pop(value_type &result)
			{
				boost::unique_lock<boost::mutex> lock(_mutex);

				while ( _container.empty() )
				{
					_condition.wait(lock);
				}

				do_pop(result);
				return result;
			}

			/*!
			 * Gets and removes an element from the front of the queue. If the
			 * queue is empty this function blocks until a new element is pushed
			 * into the stack, or until \p timeout_ms milliseconds has passed.
			 *
			 * \param result Set with the element being popped.
			 *
			 * \return \c true if an element has been popped, \c false if the queue
			 * is still empty after the given timeout
			 */
			bool pop(value_type &result, size_t timeout_ms)
			{
				boost::unique_lock<boost::mutex> lock(_mutex);

				if ( _container.empty() )
				{
					boost::system_time deadline = boost::get_system_time() +
							boost::posix_time::milliseconds(timeout_ms);
					while ( _container.empty() )
					{
						if ( !_condition.timed_wait(lock, deadline) )
						{
							return false;
						}
					}
				}

				do_pop(result);
				return true;
			}

			/*!
			 * Clears the queue, i.e. removes all elements
			 */
			void clear()
			{
				container_type tmp;
				boost::lock_guard<boost::mutex> lock(_mutex);
				std::swap(_container, tmp);
			}

		private:

			void do_push(const value_type &element)
			{
				++_size;
				_container.push_back(element);
				_condition.notify_one();
			}

			void do_pop(value_type &result)
			{
				result = _container.front();
				_container.pop_front();
				--_size;
			}

			atomic_counter       _size;
			container_type       _container;
			mutable boost::mutex _mutex;
			boost::condition     _condition;
		};
	}
}

#endif // ichramm_utils_concurrent_queue_hpp__

