#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <assert.h>

namespace saito {

typedef std::string     string_t;
typedef const string_t  cstring_t;
typedef std::string     db_serial_t;
typedef std::string     io_serial_t;

namespace cfg {
}

namespace base {

/*
 * describ:
 * _base<val_t> - just a static factory method defined
 * _base_assigned<val_t, _flag> - flag is a static identifier for all instance
 * _base_safe<val_t> - add mutex for _base<val_t>
 * _base_assigned_safe<val_t, _flag> - add mutex for _base_assgined<val_t, _flag>
 * _bai<val_t> - all _base* actions are defined here for ORM
 *
 * how-to-use:
 * to generate a class with safe demand and need to identify instances globally,
 * and with common methods for ORM
 *
 * class myclass
 *      : public _base_assigned<myclass, "myclass">,
 *        public _bai<myclass> {
 *      // type defines of myclass::key_t, db_key_t, cache_key_t is required
 *      // and interfaces in `_bai` must be implemented.
 * };
 *
 */

template<class val_t>
class _base
        : public std::enable_shared_from_this<_base<val_t>> {
public:
    static std::shared_ptr<val_t>
    new_instance () noexcept {
        std::shared_ptr<val_t> ret(new val_t);
        return ret;
    }
};

template<class val_t, const char* _flag>
class _base_assigned
        : public std::enable_shared_from_this<_base_assigned<val_t, _flag>> {
public:
    static std::shared_ptr<val_t>
    new_instance () noexcept {
        std::shared_ptr<val_t> ret(new val_t);
        return ret;
    }
    static const char*
    identifier () { return _flag; }
};

template<class val_t>
class _base_safe
        : public _base<val_t> {
    std::recursive_mutex m_mtx;
};

template<class val_t, const char* _flag>
class _base_assigned_safe
        : public _base_assigned<val_t, _flag> {
    std::recursive_mutex m_mtx;
};

template<class val_t>
class _bai {
    static_assert(std::is_empty<val_t>::key_t,
                  "key_t is not defined");
    static_assert(std::is_empty<val_t>::db_key_t,
                  "db_key_t is not defined");
    static_assert(std::is_empty<val_t>::cache_key_t,
                  "cache_key_t is not defined");

    typedef typename val_t::key_t       key_t;
    typedef typename val_t::db_key_t    db_key_t;
    typedef typename val_t::cache_key_t cache_key_t;

    // object's identifier
    virtual const key_t&
    key () noexcept = 0;
    virtual const db_key_t&
    db_key () noexcept = 0;
    virtual const cache_key_t&
    cache_key () noexcept = 0;

    // serailization
    // generate serialzed data for database and network
    virtual void
    db_serialize (db_serial_t& ret) noexcept = 0;
    virtual void
    io_serialize (io_serial_t& ret) noexcept = 0;

    // maintain indexing at runtime(memory)
    // the following two operations mostly are invoked when instance created or
    // released, for maintaining runtime status.
    virtual void
    add_index () noexcept = 0;
    virtual void
    del_index () noexcept = 0;

    // database
    // the following three operations are for maintaining database, based on
    // record particles
    virtual bool
    db_update () noexcept = 0;
    virtual bool
    db_insert () noexcept = 0;
    virtual bool
    db_delete () noexcept = 0;

    // cache
    // the following there operations are for maintaining cache, based on record
    // particles
    virtual bool
    cache_update () noexcept = 0;
    virtual bool
    cache_insert () noexcept = 0;
    virtual bool
    cache_remove () noexcept = 0;

    // uninitialize
    // add all cleaning operations
    virtual
    ~_bai () = 0;

    // @todo try to maintain database and cache based on field particles
};

template<class val_t>
class _base_mgr
        : public std::enable_shared_from_this<_base_mgr<val_t>> {
    // @todo some static check
    //static_assert(std::is_empty<val_t>::key_t,
    //              "key_t is not defined");
    //static_assert(std::is_empty<val_t>::db_key_t,
    //              "db_key_t is not defined");
    //static_assert(std::is_empty<val_t>::cache_key_t,
    //              "cache_key_t is not defined");
    //static_assert(std::is_empty<val_t>::identifier,
    //              "identifier is not assigned");
    //static_assert(
    //        std::is_base_of<val_t, _base_assigned<>>::value == true ||
    //        std::is_base_of<val_t, _base_assigned_safe<>>::value == true,
    //        "template parameter is not valid");

public:
    typedef typename val_t::key_t               key_t;
    typedef std::shared_ptr<val_t>              val_pt;
    typedef std::map<key_t, val_pt>             data_t;
    typedef std::shared_ptr<_base_mgr<val_t>>   instance_t;

    static instance_t
    instance () {
        instance_t ret(new _base_mgr<val_t>());
        return ret;
    }

private:
    static instance_t
    m_instance;
    std::recursive_mutex
    m_mtx;
    data_t
    m_data;

public:
    bool
    add (const key_t& k, val_pt v) {
        std::lock_guard<std::recursive_mutex> l(m_mtx);
        auto it = m_data.find(k);
        if (it != m_data.end())
            return false;
        m_data.insert(std::make_pair(k, v));
        return true;
    }
    val_pt
    get (const key_t& k) {
        std::lock_guard<std::recursive_mutex> l(m_mtx);
        auto it = m_data.find(k);
        if (it == m_data.end())
            return nullptr;
        return it->second;
    }
    void
    del (const key_t& k) {
        std::lock_guard<std::recursive_mutex> l(m_mtx);
        m_data.erase(k);
    }
    /* get key throuth val_t
    bool
    add (ptr_t v) {
        std::lock_guard<std::recursive_mutex> l(m_mtx);
        if (v->key()) {
            auto& k = v->key();
            return add(k, v);
        }
        return false;
    }
    void
    del (ptr_t v) {
        std::lock_guard<std::recursive_mutex> l(m_mtx);
        if (v->key()) {
            auto k = v->key();
            del(k);
        }
    }
    */
};

// template test
namespace test {

const char identifier[] = "name";

class base_test
        : public _base_assigned_safe<base_test, identifier> {
public:
    typedef int key_t;
    typedef int db_key_t;
    typedef int cache_key_t;
private:
    int m_val {0};
public:
    int
    val () { return m_val; }
    void
    val (int v) { m_val = v; }
};

class orm_test : public _base_mgr<base_test> {};

void
test () {
    auto op = base_test::new_instance ();
    op->val(100);

    auto orm_ins = orm_test::instance ();
    orm_ins->add (0, base_test::new_instance());
    orm_ins->add (1, op);

    for (int i = 0; i < 3; ++i) {
        auto ins = orm_ins->get(i);
        std::cout << i << " :::::::" << std::endl;
        assert(ins != nullptr);
        std::cout << ins->val() << std::endl;
        ins->val(ins->val() + 100);
        std::cout << ins->val() << std::endl;
    }
}

}

} // namespace base

namespace logic {

}

} // namespace saito
