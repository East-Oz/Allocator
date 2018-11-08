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

    char* buffer    = nullptr;
    size_t offset   = 0;

    pointer allocate( std::size_t n )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << "[n = " << n << "]" << std::endl;
#endif

        if( n > MaxCount )
        {
            throw std::bad_alloc();
        }

        if( buffer == nullptr )
            buffer = static_cast<char*>(std::malloc( MaxCount * sizeof( value_type ) ));

        if( buffer == nullptr )
        {
            throw std::bad_alloc();
        }

        char* item_addr = buffer + offset;
        offset += n * sizeof(value_type);
        return reinterpret_cast<pointer>( item_addr );
    }

    void deallocate( pointer p, std::size_t n )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << "[n = " << n << "]" << std::endl;
#endif
        if( buffer != nullptr )
        {
            offset -= n*sizeof(value_type);
            if( offset == 0 )
            {
                std::free( p );
                buffer = nullptr;
            }
        }

    }

    template<typename U, typename ...Args>
    void construct( U *p, Args &&...args )
    {
#ifdef SHOW_DEBUG_INFO
        std::cout << __PRETTY_FUNCTION__ << std::endl;
#endif
        new( p ) U( std::forward<Args>( args )... );
    }

    void destroy( T *p )
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
    List()
    {
        m_head = nullptr;
        m_root = nullptr;
    }
    ~List()
    {
        auto node = m_root;
        while( node )
        {
            auto _node = node->m_next;
            m_allocator.destroy(_node);
            m_allocator.deallocate(_node,1);
            node = _node;
        }
    }

    void append( const T& t )
    {

        auto node = m_allocator.allocate(1);
        m_allocator.construct(node, t);

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
        cout << to_string( c.first ) << " " << to_string( c.second ) << endl;
    }

    // custom container
    auto list1 = List<int>{};
    for( int i = 0; i < COUNT_LIMIT; i++ )
    {
        list1.append( i );
    }

    // show container
    for( auto c : list1 )
    {
        cout << to_string( c ) << endl;
    }

    {
        auto list2 = List<int, Allocator<int, COUNT_LIMIT>>{};
        for( int i = 0; i < COUNT_LIMIT; i++ )
        {
            list2.append( i );
        }

        for( auto c : list2 )
        {
            cout << to_string( c ) << endl;
        }
    }

    return 0;
}


