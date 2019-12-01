#ifndef LIB_SCP_QLIST
#define LIB_SCP_QLIST

#include <stdlib.h>  // for size_t

template<class T>
class QList {
public:
    QList() : m_end(nullptr), m_len(0), m_start(nullptr) {} // Class constructor
    ~QList() { clear(); }                                   // Class destructor

    void push_back(const T& i) { // Push item at the back of list
        node* tmp = new node(i);
        if (m_end == nullptr) { // If list is empty
            m_start = tmp;
            m_end = tmp;
        } else { // Insert at the end
            tmp->prev = m_end;
            m_end->next = tmp;
            m_end = tmp;
        }
        m_len++; // Increase size counter
    }
    
    void push_front(const T i) { // Push item at the front of the list
        node* tmp = new node(i);
	    if (m_start == nullptr) { // If list is empty
            m_start = tmp;
            m_end = tmp;
        } else { // Insert at start
            tmp->next = m_start;
            m_start->prev = tmp;
            m_start = tmp;
        }
        m_len++; // Increase size counter
    }
    
    void pop_back() { // Pops item from back 
        if (m_end != nullptr) {
            node* tmp = m_end;
            m_end = m_end->prev;
            if (m_end != nullptr) //Re-link previous item to NULL
                m_end->next = nullptr;
            else // List became empty so we need to clear start
                m_start = nullptr;
            delete tmp;
            m_len--; // Decrease counter
        }
    }
    
    void pop_front() { // Pops item from front
        if (m_start != nullptr) {
            node* tmp = m_start;
            m_start = m_start->next;
            if (m_start != nullptr) // Re-link next item to NULL
                m_start->prev = nullptr;
            else // List became empty so we need to clear end
                m_end = nullptr;
            delete tmp;
            m_len--; // Decrease counter
        }
    }
    
    T& front() const { // get item from front
        if (m_start != nullptr)
            return m_start->item;
        //TODO: Catch error when list is empty
    }
    
    T& back() const { // get item from back
        if (m_end != nullptr)
            return m_end->item;
        //TODO: Catch error when list is empty
    }
    
    inline size_t size() const { return m_len; } // Returns size of list
    
    void clear() { // Clears list
        node* tmp = m_start;
        while (m_start != nullptr) {
            tmp = m_start;
            m_start = m_start->next;
            delete tmp; // Delete item
            m_len--; // Decrease counter
        }
        m_end = nullptr;
    }
    
    void clear(unsigned int index) { // Clears list
        node* tmp = m_start;
        for (size_t i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index) {
                if (tmp->prev != nullptr)
                    tmp->prev->next = tmp->next;
                else
                    m_start = tmp->next;

                if (tmp->next != nullptr)
                    tmp->next->prev = tmp->prev;
                else
                    m_end = tmp->prev;

                m_len--; // Decrease counter
                delete tmp; // Delete item
                break;
            } else {
                tmp = tmp->next;
            }
        }
    }
    
    T get(unsigned int index) { // Get item at given index 
        node* tmp = m_start;
        for (size_t i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index)
                return tmp->item;
            else
            tmp = tmp->next;
        }
        //TODO: Catch error when index is out of range
    }
    
    T& at(unsigned int index) { // Get item at given index 
        node* tmp = m_start;
        for (size_t i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index)
                return tmp->item;
            else
                tmp = tmp->next;
        }
        //TODO: Catch error when index is out of range
    }
    
    void set(unsigned int index, T& entry) {
        if (index >= m_len) {
            push_back(entry);
        } else {
            at(index) = entry;
        }
    }
    
    bool removeAt(unsigned int index) {
        node* tmp = m_start;
        for (size_t i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index) {
                if (tmp->prev != nullptr)
                    tmp->prev->next = tmp->next;
                else
                    m_start = tmp->next;
                
                if (tmp->next != nullptr)
                    tmp->next->prev = tmp->prev;
                else
                    m_end = tmp->prev;
                delete tmp;    
                return true;
            } else {
                tmp = tmp->next;
            }
        }
        return false;
    }

    // Array operator
    T& operator[](unsigned int index) {
        node* tmp = m_start;
        for (size_t i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index)
                return tmp->item;
            else
                tmp = tmp->next;
        }
        //TODO: Catch error when index is out of range
    }   
    
    const T& operator[](unsigned int index) const { // Not realy needed
        node* tmp = m_start;
        for (int i = 0; i <= index && tmp != nullptr; i++) {
            if (i == index)
                return tmp->item;
            else
                tmp = tmp->next;
        }
        //TODO: Catch error when index is out of range
    }
    
    // Non - critical functions
    int indexOf(const T& val) const {
        for (size_t i = 0; i < size(); i++)
            if (at(i) == val)
                return i;
        return -1;
    }
    
private:
    typedef struct node {
        T     item;
        node* next;
        node* prev;
        
        node(const T& i) : item(i), next(nullptr), prev(nullptr) {}
    } node;

    node*  m_end; // Pointers to start and end
    size_t m_len; // Size of list 
    node*  m_start;
};

#endif