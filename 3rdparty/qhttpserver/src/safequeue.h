#ifndef SAFEQUEUE
#define SAFEQUEUE

// http://gnodebian.blogspot.hu/2013/07/a-thread-safe-asynchronous-queue-in-c11.html


#include <queue>
#include <list>
#include <mutex>
#include <thread>
#include <cstdint>
#include <condition_variable>



/** A thread-safe asynchronous queue */
template <class T, class Container = std::list<T> >
class SafeQueue : std::queue<T, Container>
{

    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;
    typedef Container container_type;
    typedef std::queue<T, Container> Super;

    // m_queue
    inline Super const & queue() const { return *this; }
    inline Super & queue() { return *this; }

    struct iterator {
        typename Container::iterator it;
        std::mutex *lock;
        bool root;
        iterator(typename Container::iterator const & it, SafeQueue<T,Container> &sq) :
            it(it), lock(&sq.m_mutex), root(1) {
            lock->lock();
        }
        iterator(iterator &&o) {
            it = o.it;
            lock = o.lock;
            root = o.root;
            o.root = 0;
        }
        iterator(iterator const & o) {
            it = o.it;
            lock = o.lock;
            root = 0;
        }

        ~iterator() {
            if (root) {
                lock->unlock();
            }
        }
        iterator & operator++() {
            ++it;
            return *this;
        }
        iterator operator++(int) const {//postfix
            iterator i2 = *this;
            ++i2.it;
            return i2;
        }
        T& operator* () const {
            if (!root) {
                throw std::exception();
            }
            return *it;
        }
        bool operator ==(iterator const & o) const {
            return it == o.it;
        }
        bool operator !=(iterator const & o) const {
            return it != o.it;
        }
    };
    struct const_iterator {
        typename Container::const_iterator it;
        std::mutex *lock;
        bool root;
        const_iterator(typename Container::const_iterator const & it, SafeQueue<T,Container> &sq) :
            it(it), lock(&sq.m_mutex), root(1) {
            lock->lock();
        }
        const_iterator(const_iterator &&o) {
            it = o.it;
            lock = o.lock;
            root = o.root;
            o.root = 0;
        }
        const_iterator(const_iterator const & o) {
            it = o.it;
            lock = o.lock;
            root = 0;
        }

        ~const_iterator() {
            if (root) {
                lock->unlock();
            }
        }

        const_iterator & operator++() {
            ++it;
            return *this;
        }
        const_iterator operator++(int) const {//postfix
            const_iterator i2 = *this;
            ++i2.it;
            return i2;
        }
        T const & operator* () const {
            if (!root) {
                throw std::exception();
            }
            return *it;
        }
        bool operator ==(const_iterator const & o) const {
            return it == o.it;
        }
        bool operator !=(const_iterator const & o) const {
            return it != o.it;
        }
    };

  public:

    /*! Create safe queue. */
    SafeQueue() = default;
    SafeQueue (SafeQueue&& sq)
    {
      *this = std::move (sq.queue());
    }
    SafeQueue (const SafeQueue& sq)
    {
      std::lock_guard<std::mutex> lock (sq.m_mutex);
      queue() = sq.queue();
    }

    /*! Destroy safe queue. */
    ~SafeQueue()
    {
      std::lock_guard<std::mutex> lock (m_mutex);
    }

    const_iterator begin() const {
        return const_iterator(Super::c.begin(),*this);
    }

    const_iterator end() const {
        return const_iterator(Super::c.end(),*this);
    }

    iterator begin() {
        return iterator(Super::c.begin(),*this);
    }

    iterator end() {
        return iterator(Super::c.end(),*this);
    }

    /**
     * Sets the maximum number of items in the queue. Defaults is 0: No limit
     * \param[in] item An item.
     */
    void set_max_num_items (unsigned int max_num_items)
    {
      m_max_num_items = max_num_items;
    }

    /**
     *  Pushes the item into the queue.
     * \param[in] item An item.
     * \return true if an item was pushed into the queue
     */
    bool push (const value_type& item)
    {
      std::lock_guard<std::mutex> lock (m_mutex);

      if (m_max_num_items > 0 && queue().size() > m_max_num_items)
        return false;

      queue().push (item);
      m_condition.notify_one();
      return true;
    }

    /**
     *  Pushes the item into the queue.
     * \param[in] item An item.
     * \return true if an item was pushed into the queue
     */
    bool push (const value_type&& item)
    {
      std::lock_guard<std::mutex> lock (m_mutex);

      if (m_max_num_items > 0 && queue().size() > m_max_num_items)
        return false;

      queue().push (item);
      m_condition.notify_one();
      return true;
    }

    /**
     *  Pops item from the queue. If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void pop (value_type& item)
    {
      std::unique_lock<std::mutex> lock (m_mutex);
      m_condition.wait (lock, [this]() // Lambda funct
      {
        return !queue().empty();
      });
      item = queue().front();
      queue().pop();
    }

    /**
     *  Pops item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the pop() method if that type has no move assignment operator.
     *  If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void move_pop (value_type& item)
    {
      std::unique_lock<std::mutex> lock (m_mutex);
      m_condition.wait (lock, [this]() // Lambda funct
      {
        return !queue().empty();
      });
      item = std::move (queue().front());
      queue().pop();
    }

    /**
     *  Tries to pop item from the queue.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_pop (value_type& item)
    {
      std::unique_lock<std::mutex> lock (m_mutex);

      if (queue().empty())
        return false;

      item = queue().front();
      queue().pop();
      return true;
    }

    /**
     *  Tries to pop item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the try_pop() method if that type has no move assignment operator.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_move_pop (value_type& item)
    {
      std::unique_lock<std::mutex> lock (m_mutex);

      if (queue().empty())
        return false;

      item = std::move (queue().front());
      queue().pop();
      return true;
    }

    /**
     *  Pops item from the queue. If the queue is empty, blocks for timeout microseconds, or until item becomes available.
     * \param[out] t An item.
     * \param[in] timeout The number of microseconds to wait.
     * \return true if get an item from the queue, false if no item is received before the timeout.
     */
    bool timeout_pop (value_type& item, std::uint64_t timeout)
    {
      std::unique_lock<std::mutex> lock (m_mutex);

      if (queue().empty())
        {
          if (timeout == 0)
            return false;

          if (m_condition.wait_for (lock, std::chrono::microseconds (timeout)) == std::cv_status::timeout)
            return false;
        }

      item = queue().front();
      queue().pop();
      return true;
    }

    /**
     *  Pops item from the queue using the contained type's move assignment operator, if it has one..
     *  If the queue is empty, blocks for timeout microseconds, or until item becomes available.
     *  This method is identical to the try_pop() method if that type has no move assignment operator.
     * \param[out] t An item.
     * \param[in] timeout The number of microseconds to wait.
     * \return true if get an item from the queue, false if no item is received before the timeout.
     */
    bool timeout_move_pop (value_type& item, std::uint64_t timeout)
    {
      std::unique_lock<std::mutex> lock (m_mutex);

      if (queue().empty())
        {
          if (timeout == 0)
            return false;

          if (m_condition.wait_for (lock, std::chrono::microseconds (timeout)) == std::cv_status::timeout)
            return false;
        }

      item = std::move (queue().front());
      queue().pop();
      return true;
    }

    inline bool removeOne(T const & theOne) {
        for (auto it = begin(); it!=end(); it++) {
            if (*it==theOne) {
                Super::c.erase(it.it);
                return true;
            }
        }
        return false;
    }

    /**
     *  Gets the number of items in the queue.
     * \return Number of items in the queue.
     */
    size_type size() const
    {
      std::lock_guard<std::mutex> lock (m_mutex);
      return queue().size();
    }

    /**
     *  Check if the queue is empty.
     * \return true if queue is empty.
     */
    bool empty() const
    {
      std::lock_guard<std::mutex> lock (m_mutex);
      return queue().empty();
    }

    /**
     *  Swaps the contents.
     * \param[out] sq The SafeQueue to swap with 'this'.
     */
    void swap (SafeQueue& sq)
    {
      if (this != &sq)
        {
          std::lock_guard<std::mutex> lock1 (m_mutex);
          std::lock_guard<std::mutex> lock2 (sq.m_mutex);
          queue().swap (sq.queue());

          if (!queue().empty())
            m_condition.notify_all();

          if (!sq.queue().empty())
            sq.m_condition.notify_all();
        }
    }

    /*! The copy assignment operator */
    SafeQueue& operator= (const SafeQueue& sq)
    {
      if (this != &sq)
        {
          std::lock_guard<std::mutex> lock1 (m_mutex);
          std::lock_guard<std::mutex> lock2 (sq.m_mutex);
          std::queue<T, Container> temp {sq.queue()};
          queue().swap (temp);

          if (!queue().empty())
            m_condition.notify_all();
        }

      return *this;
    }

    /*! The move assignment operator */
    SafeQueue& operator= (SafeQueue && sq)
    {
      std::lock_guard<std::mutex> lock (m_mutex);
      queue() = std::move (sq.queue());

      if (!queue().empty())  m_condition.notify_all();

      return *this;
    }


  private:

    //std::queue<T, Container> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    unsigned int m_max_num_items = 0;
};

/*! Swaps the contents of two SafeQueue objects. */
template <class T, class Container>
void swap (SafeQueue<T, Container>& q1, SafeQueue<T, Container>& q2)
{
  q1.swap (q2);
}

#endif // SAFEQUEUE

