/*
 * Copyright 2015 TU Chemnitz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include <cassert>
#include <functional>

/// bidirectional list node
struct node
{
  node *m_prev, *m_next;
};

template <class T> class observable;

/// observer to an observable variable
class observer
  : private node
{

  template <class T> friend class observable;

  const std::function<void ()> m_callback; ///< what to do when variable is set

public:

  /// create and register observer to observable variable
  template <class T>
  observer(observable<T> &o, std::function<void ()> &&callback)
    : node{o.m_prev, &o}, m_callback{callback}
  {
    m_prev->m_next = this;
    m_next->m_prev = this;
  }

  /// destroy and unregister observer
 ~observer()
  {
    m_prev->m_next = m_next;
    m_next->m_prev = m_prev;
  }

  observer(const observer &) = delete;
  observer &operator=(const observer &) = delete;

};

/// observable variable
template <class T>
class observable
  : private node
{

  friend class observer;

  T m_v; ///< current value of variable

  ///< check whether there are no observers
  bool has_observers() const
  {
    return m_next != this;
  }

public:

  /// create observable variable
  observable(const T &v = T())
    : node{this, this}, m_v{v}
  { }

  /// destroy observable variable
 ~observable()
  {
    assert(!has_observers());
  }

  observable(const observable &) = delete;
  observable &operator=(const observable &) = delete;

  /// give current value
  operator const T &() const
  {
    return m_v;
  }

  /// assign new value
  const T &operator=(const T &v)
  {
    m_v = v;
    for (node *o = m_next; o != this; o = o->m_next)
      static_cast<observer *>(o)->m_callback();

    return v;
  }

};

#endif // inclusion guard
