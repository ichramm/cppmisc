#ifndef buffer_hpp__
#define buffer_hpp__

#include <boost/shared_ptr.hpp>


namespace transport {


    /**
     * This is going to be a layered buffer someday..
     */
    template <class T, size_t size = 1024>
    class buffer {



    private:
        typedef std::array<T, size> bucket;
        typedef bucket* bucket_ptr;
        std::vector<bucket_ptr> buffer_;
    };


}

#endif // buffer_hpp__
