#pragma once

#include "smart_list/smart_list.h"

/// @brief This class is defines a payload manager
/// that is used to link entity drawn to logical
/// entities
/// This is necessary to abstract the handling
/// of the payload
class AbstractEntityPayload {
public:
	virtual ~AbstractEntityPayload() {}
	virtual void add_payload() = 0;
	virtual void free_payload(int idx_p) = 0;
};

/// @brief a no op payload (not containing anything)
class NoOpEntityPayload : public AbstractEntityPayload {
public:
	void add_payload() override {}
	void free_payload(int) override {}
};

/// @brief a basic template payload
template<class T>
class EntityPayload : public AbstractEntityPayload {
public:
	void add_payload() override { _list.new_instance(T()); }
	void free_payload(int idx_p) override { _list.free_instance(idx_p); }

	T& get_payload(int idx_p)
	{
		return _list.get(idx_p);
	}

	T const & get_payload(int idx_p) const
	{
		return _list.get(idx_p);
	}

private:
	smart_list<T> _list;
};
