
#ifndef _CACHE_H__INCLUDED_
#define _CACHE_H__INCLUDED_
#include <memory>
#include <mutex>

#include "cacheimpl.h"


namespace utility {

struct single_thread {};
struct multi_thread  {};

struct true_type	{};
struct false_type	{};

} // namespace utility



namespace common {

template<
	class KeyType, 
	class ValueType,
	class ThreadMode = utility::single_thread,
	template<class _k, class _v> class CacheTemplate = lru_cache
>
class Cache;

template<
	class KeyType, 
	class ValueType,
	template<class _k, class _v> class CacheTemplate
>
class Cache<KeyType, ValueType, utility::single_thread, CacheTemplate>
{
public:	
	typedef KeyType													key_type;
	typedef ValueType												value_type;
	typedef utility::single_thread									thread_mode;
	typedef CacheTemplate<key_type, value_type>						cache_type;
	typedef Cache<key_type, value_type, thread_mode, CacheTemplate> this_type;

private:
	typedef std::auto_ptr<cache_type> _impl_holder;

public:	
	Cache(unsigned size) : 
		_impl(new cache_type(size))
	{
	}

	Cache(const this_type& rhs) : 
		_impl(rhs._impl)
	{
	}

	this_type& operator=(const this_type& rhs)
	{
		_impl = rhs._impl;
		return *this;
	}

public:
	bool get(const key_type &key, value_type& val)
	{
		return _impl->get(key, val);
	}

	void set(const key_type &key, const value_type& val)
	{
		_impl->set(key, val);
	}

	bool pop(value_type& val)
	{
		return _impl->pop(val);
	}

	bool pull(value_type& val)
	{
		return _impl->pull(val);
	}

	bool remove(const key_type &key)
	{
		return _impl->remove(key);
	}

	template <typename predicate>
	unsigned remove(predicate pred)
	{
		return _impl->remove(pred);
	}

	void clear()
	{
		value_type val;
		while (_impl->pop(val)) ;
	}

	bool is_full()
	{
		return _impl->is_full();
	}

private:
	_impl_holder _impl;
};


template<
	class KeyType, 
	class ValueType,
	template<class _k, class _v> class CacheTemplate
>
class Cache<KeyType, ValueType, utility::multi_thread, CacheTemplate>
{
public:	
	typedef KeyType													key_type;
	typedef ValueType												value_type;
	typedef utility::multi_thread							thread_mode;
	typedef CacheTemplate<key_type, value_type>						cache_type;
	typedef Cache<key_type, value_type, thread_mode, CacheTemplate> this_type;
	typedef	Cache<KeyType, ValueType, utility::single_thread, CacheTemplate>	impl_type;

public:	
	Cache(unsigned size) : 
		_impl(size)
	{
	}

	Cache(const this_type& rhs) : 
		_impl(rhs._impl)
	{
	}

	this_type& operator=(const this_type& rhs)
	{
		_impl = rhs._impl;
		return *this;
	}

public:
	bool get(const key_type &key, value_type& val)
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.get(key, val);
	}

	void set(const key_type &key, const value_type& val)
	{
		std::lock_guard<std::mutex> guard(_lock);
		_impl.set(key, val);
	}

	bool pop(value_type& val)
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.pop(val);
	}

	bool pull(value_type& val)
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.pull(val);
	}

	bool remove(const key_type &key)
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.remove(key);
	}

	template <typename predicate>
	unsigned remove(predicate pred)
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.remove(pred);
	}

	void clear()
	{
		std::lock_guard<std::mutex> guard(_lock);
		value_type val;
		while (_impl.pop(val)) ;
	}

	bool is_full()
	{
		std::lock_guard<std::mutex> guard(_lock);
		return _impl.is_full();
	}

private:
	impl_type					_impl;
	std::mutex _lock;
};

} // namespace common

#endif

