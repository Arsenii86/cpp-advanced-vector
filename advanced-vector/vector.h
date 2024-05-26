#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <iostream>
template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }
    
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept { 
        *this=std::move(other);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept { 
        buffer_=std::move(rhs.buffer_);
        capacity_=rhs.capacity_;
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        //std::cout<<index<<"  "<<capacity_<<std::endl;
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};



template <typename T>
class Vector {
public:
    
    Vector() = default;
    
     using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept{
        return data_.GetAddress();
    };
    iterator end() noexcept{
        return data_.GetAddress()+size_;
    };
    const_iterator begin() const noexcept{
        return data_.GetAddress();
    };
    const_iterator end() const noexcept{
        return data_.GetAddress()+size_;
    };
    const_iterator cbegin() const noexcept{
        return data_.GetAddress();
        
    };
    const_iterator cend() const noexcept{
        return data_.GetAddress()+size_;
       
    };
    

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

   
    Vector(const Vector& other)
        : data_(other.size_)        
        , size_(other.size_)  
    {        
         std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());            
    }  
    
    Vector(Vector&& other) noexcept{     
        Swap(other);
    }    
    
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                /* Применить copy-and-swap */
                Vector rhs_copy(rhs);
                Swap(rhs_copy);                 
            } 
            else {
                   /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                size_t min_size =std::min(size_,rhs.size_);
                for(size_t i=0; i<min_size; i++){                      
                    data_[i]=rhs.data_[i];                        
                }
                if(rhs.size_<size_){                   
                   std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_); 
                }                
                else {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress());
                }
                size_=rhs.size_;
            }
        }
        return *this;
    }
    
    Vector& operator=(Vector&& rhs) noexcept{       
        Swap(rhs);
        return *this;
    };
    
    void Swap(Vector& rhs) noexcept{
        data_.Swap(rhs.data_);
        std::swap(size_,rhs.size_);
         
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        CopyOrMove (data_.GetAddress(),new_data.GetAddress(),size_);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);       
    }    
    
    void Resize(size_t new_size){
        if(new_size < size_){
            std::destroy_n(data_.GetAddress()+new_size, size_- new_size);
            size_ = new_size;
        }
        else{
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
    }
    
    void PushBack(const T& value){        
        EmplaceBack(value); 
    }
    
    void PushBack(T&& value) {        
        EmplaceBack(std::move(value));        
    }
    
    void PopBack(){
        std::destroy_n(data_.GetAddress()+(size_-1), 1);        
        size_--;
    } 
    
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);        
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args){
       if (size_ == Capacity()) {
            return *InputYesRelocation(data_ + size_,std::forward<Args>(args)...);
        } 
        else{            
            new (data_ + size_) T(std::forward<Args>(args)...);
            size_+=1;
            return data_[size_-1];
        }       
    }
    
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){        
        if (size_ < Capacity()){              
            return InputNoRelocation(pos,std::forward<Args>(args)...);
        }
        else{
            return InputYesRelocation(pos,std::forward<Args>(args)...);
            
        }
    }
    
    iterator Insert(const_iterator pos, const T& value){
        return Emplace(pos, value );
    }
    
    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos,std::move(value) );
    }
    
    iterator Erase(const_iterator pos){
        auto pos_non_const = const_cast<T*>(pos);
        std::move( pos_non_const+1, end(), pos_non_const );
        std::destroy_n(end()-1,1);
        --size_;
        if (size_==0) return end();
        else return pos_non_const;        
    }
    
    
    
    
private:
    
    template<typename FirstIter, typename SecondIter>
    void CopyOrMove (FirstIter from, SecondIter to, size_t number){
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, number, to);
        } else {
            std::uninitialized_copy_n(from, number, to);
        }
    } 
    
    template<typename FirstIter, typename SecondIter,typename CleanIter>
    void CleanCopyOrMove(FirstIter from, SecondIter to, size_t number_to_move_copy, CleanIter clean_from, size_t number_to_clean){
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(from, number_to_move_copy, to);
            } 
            else {
                try{
                    std::uninitialized_copy_n(from, number_to_move_copy, to);
                }
                catch(...){
                    std::destroy_n(clean_from, number_to_clean);
                }
            }            
    }
        
       
    template <typename... Args>
    iterator InputNoRelocation(const const_iterator pos, Args&&... args){
        auto pos_non_const = const_cast<T*>(pos);
            if (pos==end()){               
                new (pos_non_const) T(std::forward<Args>(args)...);               
            }
            else {                
                T temp_val(std::forward<Args>(args)...);
                CopyOrMove (end()-1,end(),1);
                std::move_backward(pos_non_const, end()-1, end()); 
                *pos_non_const  = std::move(temp_val);               
            }            
            ++size_;
            return pos_non_const;
    }    
    
    template <typename... Args>
    iterator InputYesRelocation(const const_iterator pos, Args&&... args){
    auto pos_non_const = const_cast<T*>(pos);            
            Vector<T> new_data(0);
            if (size_==0) new_data.Reserve(1);
            else new_data.Reserve(size_*2);
            new_data.size_ = size_;
            int dist_before = pos_non_const - begin();
            int dist_after = end()- pos_non_const;
            auto iter = new_data.data_.GetAddress() + dist_before;
            new (iter) T(std::forward<Args>(args)...);           
            CleanCopyOrMove(begin(), new_data.data_.GetAddress(),dist_before, iter, 1); 
            CleanCopyOrMove(pos_non_const,  iter+1, dist_after, new_data.data_.GetAddress(), dist_before+1);
            Swap(new_data);
            ++size_;            
            return (begin()+dist_before);     
    }
       
    RawMemory<T> data_;    
    size_t size_ = 0;   
    
};