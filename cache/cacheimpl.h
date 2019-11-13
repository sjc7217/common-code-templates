
#include <list>
#include <algorithm>

#include <unordered_map>
// #include <ext/hash_fun.h>

//////////////////////////////////////////////////////////////////////////

namespace common {

template<class K, class T>
class CacheImpl
{
	typedef std::unordered_map<K, T> _map_type;
public:
	CacheImpl(unsigned int cap) : 
		cache_cap(cap), cache_size(0)
	{
	}
	CacheImpl() {}

	virtual ~CacheImpl()
	{
	}

	bool get(const K& key, T& val)
	{
		typename _map_type::const_iterator iter 
			= h_map.find(key);

		if (iter == h_map.end()) {
			return false;
		}
		else {
			val = iter->second;
			update_cache(key);
			return true;
		}		
	}

	void set(const K& key, const T& val)
	{
		typename _map_type::iterator findret 
			= h_map.find(key);

		if (findret == h_map.end())
		{
			if (is_full())
				erase_old_item();
			insert_new_item(key, val);			
		}
		else
		{
			h_map[key] = val;
		}
	}

	bool pop(T& val)
	{
		if (_pop(val)) {
			cache_size--;
			return true;
		}
		else {
			cache_size = 0;
			return false;
		}
	}

	bool pull(T& val)
	{
		if (_pull(val)) {
			cache_size--;
			return true;
		}
		else {
			cache_size = 0;
			return false;
		}
	}

	bool remove(const K& key)
	{
		typename _map_type::iterator findret = 
			h_map.find(key);
		if (findret == h_map.end()) {
			return false;
		}
		else {
			h_map.erase(findret);
			erase_key(key);
			return true;
		}
	}

	template <typename predicate>
	unsigned remove(predicate pred)
	{
		const unsigned original_size = h_map.size();
		for (typename _map_type::iterator iter = h_map.begin();
			iter != h_map.end(); )
		{
			const K& key = iter->first;
			const T& value = iter->second;
			if (pred(key, value))
				h_map.erase(iter++);
			else
				++iter;
		}
		return original_size - h_map.size();
	}

	bool is_full()
	{
		return cache_size >= cache_cap;
	}

private:

	void erase_old_item()
	{
		cache_size--;
		this->_erase_old_item();
	}

	void insert_new_item(const K& key, const T& val)
	{
		cache_size++;
		this->_insert_new_item(key, val);
	}

	void update_cache(const K& key)
	{
		this->_update_cache(key);
	}

	void erase_key(const K& key)
	{
		cache_size--;
		this->_erase_key(key);
	}

private:
	virtual bool _pop(T& val)									= 0;
	virtual bool _pull(T& val)									= 0;
	virtual void _erase_old_item()								= 0;
	virtual void _insert_new_item(const K& key, const T& val)	= 0;
	virtual void _update_cache(const K& key)					= 0;
	virtual void _erase_key(const K& key)						= 0;

protected:
	unsigned int cache_size;
	unsigned int cache_cap;
	std::unordered_map<K, T> h_map;
};

//--------------------------------------------------------------------------------------------
template<class K, class T>
class lru_cache : public CacheImpl<K, T>
{
	typedef CacheImpl<K, T> base_type;
	typedef lru_cache<K, T>	this_type;
	typedef std::unordered_map<K, T> _map_type;

public:
	lru_cache(unsigned int cap) : 
		CacheImpl<K, T>(cap)
	{
	}

private:

	bool _pop(T& val)
	{
		if (element_lifes.empty())
			return false;

		typename std::unordered_map<K, time_t>::iterator max_iter = element_lifes.begin();
		for (typename std::unordered_map<K, time_t>::iterator i = max_iter;
			i != element_lifes.end(); i++)
		{
			if (i->second > max_iter->second)
				max_iter = i;
		}

		const K& k = max_iter->first;
		typename _map_type::iterator findret
			= base_type::h_map.find(k);
		val = (*findret).second;
		base_type::h_map.erase(findret);
		element_lifes.erase(max_iter);
		return true;
	}

	bool _pull(T& val)
	{
		if (element_lifes.empty())
			return false;

		typename std::unordered_map<K, time_t>::iterator min_iter = element_lifes.begin();
		for (typename std::unordered_map<K, time_t>::iterator i = min_iter;
			i != element_lifes.end(); i++)
		{
			if (i->second < min_iter->second)
				min_iter = i;
		}

		const K& k = min_iter->first;
		typename _map_type::iterator findret
			= base_type::h_map.find(k);
		val = (*findret).second;
		base_type::h_map.erase(findret);
		element_lifes.erase(min_iter);
		return true;
	}

	void _erase_old_item()
	{
		if (element_lifes.empty())
			return;

		typename std::unordered_map<K, time_t>::iterator min_iter = element_lifes.begin();
		for (typename std::unordered_map<K, time_t>::iterator i = min_iter;
			i != element_lifes.end(); i++)
		{
			if (i->second < min_iter->second)
				min_iter = i;
		}

		const K& k = min_iter->first;
		typename _map_type::iterator finret
			= base_type::h_map.find(k);
		base_type::h_map.erase(finret);
		element_lifes.erase(min_iter);
	}

	void _insert_new_item(const K& key, const T& val)
	{
		CacheImpl<K, T>::h_map[key] = val;
		element_lifes[key] = time(0);
	}

	void _update_cache(const K& key)
	{
		element_lifes[key] = time(0);
	}

	void _erase_key(const K& key)
	{
		typename std::unordered_map<K, time_t>::iterator findret = element_lifes.find(key);
		if (findret != element_lifes.end())
			element_lifes.erase(findret);	
	}

private:
	std::unordered_map<K, time_t> element_lifes;
};

//-------------------------------------------------------------------------------------------
template<class K, class T>
class lfu_cache : public CacheImpl<K, T>
{
public:
	lfu_cache(unsigned int size) :
		CacheImpl<K, T>(size)
	{
	}

private:

	bool _pop(T& val)
	{
		if(cache_list.empty())
			return false;
		K& k = cache_list.front().first;
		typename std::unordered_map<K, T*>::iterator map_iter 
			= CacheImpl<K, T>::h_map.find(k);
		CacheImpl<K, T>::h_map.erase(map_iter);
		cache_list.pop_front();
		return true;
	}

	bool _pull(T& val)
	{
		if(cache_list.empty())
			return false;
		K& k = cache_list.back().first;
		typename std::unordered_map<K, T*>::iterator map_iter 
			= CacheImpl<K, T>::h_map.find(k);
		CacheImpl<K, T>::h_map.erase(map_iter);
		cache_list.pop_back();
		return true;
	}

	void _erase_old_item()
	{
		K& k = (--cache_list.end())->first;
		typename std::unordered_map<K, T*>::iterator map_iter 
			= CacheImpl<K, T>::h_map.find(k);
		CacheImpl<K, T>::h_map.erase(map_iter);
		cache_list.pop_back();
	}

	void _insert_new_item(const K& key, const T& val)
	{
		CacheImpl<K, T>::h_map[key] = val;
		if(cache_list.empty())
		{
			cache_list.push_back(k_f(key, 1));
			return;
		}
		typename std::list<k_f>::iterator cache_iter = --cache_list.end();
		unsigned int f = cache_iter->second;
		while (f == 1 && cache_iter != cache_list.begin())
		{
			f = (--cache_iter)->second;
		}
		if (f > 1)
			cache_iter++;
		cache_list.insert(cache_iter, k_f(key, 1));
	}

	void _update_cache(const K& key)
	{
		typename std::list<k_f>::iterator src_iter = cache_list.begin();
		while(src_iter != cache_list.end())
		{
			if(src_iter->first == key)
				break;
			src_iter++;
		}
		unsigned int f = ++(src_iter->second);
		if (src_iter == cache_list.begin())
			return;
		typename std::list<k_f>::iterator dst_iter = src_iter;
		dst_iter--;
		while (f >= dst_iter->second && dst_iter != cache_list.begin())
			dst_iter--;
		if (dst_iter->second > f)
			dst_iter++;
		if (src_iter != dst_iter) 
		{
			cache_list.insert(dst_iter, k_f(key, f));
			cache_list.erase(src_iter);
		}
	}

	void _erase_key(const K& key)
	{
		typename std::list<k_f>::iterator iter = cache_list.begin();
		while(iter != cache_list.end())
		{
			if(iter->first == key)
				break;
			iter++;
		}
		cache_list.erase(iter); // the key must be exist,so do not need check the iter.
	}

private:
	typedef std::pair<K, unsigned int> k_f;
	std::list<k_f> cache_list;
};

} // namespace common

