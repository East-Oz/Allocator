#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

#define COUNT_LIMIT 10

//#define SHOW_DEBUG_INFO 1

template<typename T, size_t MaxCount>
struct Allocator
{
    using value_type        = T;
    using pointer           = T*;
    using const_pointer     = const T*;
    using reference         = T&;
    using const_reference   = const T&;

    template<typename U>
    struct rebind {
        using other = Allocator<U, MaxCount>;
    };

    char*   m_buffer    = nullptr;
    size_t  m_offset    = 0;
    size_t  m_count     = 0;

    pointer allocate( std::size_t n )
    {
#ifdef SHOW_DEBUG_INFO
        cout << __PRETTY_FUNCTION__ << "[n = " << n << "]" << endl;
#endif

        if( n > MaxCount )
        {
            throw std::bad_alloc();
        }

        if( m_buffer == nullptr )
        {
            m_buffer = static_cast<char*>(std::malloc( MaxCount * sizeof( value_type ) ));
            m_count = MaxCount;
#ifdef SHOW_DEBUG_INFO
            cout << endl;
            cout << "Allocate: " << MaxCount * sizeof( value_type ) << ", sizeof item= " << sizeof( value_type ) << std::endl;
            cout << endl;
#endif
        }

        if( m_buffer == nullptr )
        {
            throw std::bad_alloc();
        }

        char* item_addr = m_buffer + m_offset;
        m_offset += n * sizeof(value_type);
        return reinterpret_cast<pointer>( item_addr );
    }

    void deallocate( pointer p, std::size_t n )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << "[n = " << n << "]" << std::endl;
#endif
        if( m_buffer != nullptr )
        {
            m_count -= n;
            if( m_count == 0 )
            {
                std::free( p );
                m_buffer = nullptr;
#ifdef SHOW_DEBUG_INFO
            cout << endl;
            cout << "Deallocate p=" << p << std::endl;
            cout << endl;
#endif
            }
        }
    }

    template<typename U, typename ...Args>
    void construct( U* p, Args &&...args )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
        new( p ) U( std::forward<Args>( args )... );
    }

    void destroy( T* p )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
        p->~T();
    }
};

template< typename T, typename Allocator = std::allocator<T> >
class List
{
private:
    template< typename U >
    struct Node
    {
        Node(): m_next( nullptr ) {}
        Node( const T& t ): m_t( t ), m_next( nullptr ) {}

        U m_t;
        Node* m_next;
    };
    typename Allocator::template rebind<Node<T>>::other m_allocator;
    Node<T>* m_head;
    Node<T>* m_root;

public:
    class Iterator
    {
    public:
        Iterator( Node<T>* node ): m_node( node ) {}

        bool operator==( const Iterator& other ) const
        {
            if( this == &other )
            {
                return true;
            }
            return m_node == other.m_node;
        }

        bool operator!=( const Iterator& other ) const
        {
            return !operator==( other );
        }

        T operator*() const
        {
            if( m_node )
            {
                return m_node->m_t;
            }
            return T();
        }

        void operator++()
        {
            if( m_node )
            {
                m_node = m_node->m_next;
            }
        }

    private:
        Node<T>* m_node;
    };

public:
    List(): m_head(nullptr), m_root( nullptr )
    {
    }

    List( const List& other ): m_head(nullptr), m_root( nullptr )
    {
        for( auto v: other )
        {
            append(v);
        }
    }

    List( List&& other ):  m_allocator( other.m_allocator ), m_head( other.m_head ), m_root( other.m_root )
    {
        other.m_head = nullptr;
        other.m_root = nullptr;
    }

    ~List()
    {
        auto node = m_root;
        while( node )
        {
            auto _node = node->m_next;
            if( _node )
            {
                m_allocator.destroy(_node);
                m_allocator.deallocate(_node,1);
            }
            node = _node;
        }
        m_allocator.destroy(m_root);
        m_allocator.deallocate(m_root,1);
    }

    template<typename... Args>
    void append( Args&& ... args )
    {
        auto node = m_allocator.allocate(1);
        m_allocator.construct(node, std::forward <Args>(args) ...);

        if( m_root == nullptr )
        {
            m_root = node;
            m_head = m_root;
        }
        else
        {
            m_head->m_next = node;
            m_head = m_head->m_next;
        }
    }

    T head() const
    {
        return  m_head;
    }

    Iterator begin() const
    {
        return Iterator( m_root );
    }

    Iterator end() const
    {
        return Iterator( nullptr );
    }
};

int fact( int n )
{
    int res = 1;
    for( int i = 1; i <= n; ++i )
        res *= i;
    return res;
}


int main( int, char *[] )
{
    // Standart allocator
    auto map1 = map<int, int> {};
    for( int i = 0; i < COUNT_LIMIT; i++ )
    {
        map1[ i ] = fact( i );
#ifdef SHOW_DEBUG_INFO
        cout << endl;
#endif
    }

#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
#endif

    // custom allocator
    auto map2 = std::map<int, int, std::less<int>, Allocator<std::pair<const int, int>, COUNT_LIMIT>>{};
    for( int i = 0; i < COUNT_LIMIT; i++ )
    {
        map2[ i ] = fact( i );
#ifdef SHOW_DEBUG_INFO
        cout << endl;
#endif
    }
#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
#endif

    // show container
    for( auto c : map2 )
    {
        cout << c.first << " " << c.second << endl;
    }

#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
    cout << "list1" << endl;
#endif

    // custom container
    auto list1 = List<int>{};
    for( int i = 0; i < COUNT_LIMIT; i++ )
    {
        list1.append( i );
    }

    // show container
    for( auto c : list1 )
    {
        cout << c << endl;
    }

    auto list2 = List<int, Allocator<int, COUNT_LIMIT>>{};
    for( int i = 0; i < COUNT_LIMIT; i++ )
    {
        list2.append( i );
    }

#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
    cout << "list2" << endl;
#endif

    for( auto c : list2 )
    {
        cout << c << endl;
    }

#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
    cout << "list3 - moved from list2 (list2 corrupted)" << endl;
#endif

    auto list3( std::move( list2 ) );
    for( auto c : list3 )
    {
        cout << c << endl;
    }   

#ifdef SHOW_DEBUG_INFO
    cout << "--------------------------------------------------" << endl;
    cout << "list4 - copy of list1" << endl;
#endif

    auto list4(list1);
    for( auto c : list4 )
    {
        cout << c << endl;
    }

    return 0;
}


