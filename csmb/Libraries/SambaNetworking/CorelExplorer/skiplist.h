/* Name: skiplist.h

   Description: This file is a part of the Corel File Manager application.

   Author:	Jasmin Blanchette

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.


*/

#ifndef __INC_SKIPLIST_H__
#define __INC_SKIPLIST_H__

#include "common.h"

template <class T>
class CSkipList
{
public:
	class Iterator;

	CSkipList();
	~CSkipList();

	void Clear();
	BOOL Insert(const T& t);
	BOOL Remove(const T& t);
	Iterator Find(const T& t) const;
	Iterator Begin() const;
	Iterator End() const;

private:
	// disable copy and assignment
	CSkipList(const CSkipList&);
	CSkipList& operator=(const CSkipList&);

	struct SCell
	{
		T content;
		SCell **nexten;

		SCell(SCell *root, const T& t, int degree);
		SCell(SCell *root, int degree);
		~SCell();
	};
	enum { MAX_DEGREE = 15 };

	SCell m_Root;
	int m_Degree;

public:
	class Iterator
	{
		friend class CSkipList;
	public:
		Iterator();

		Iterator& operator++();
		Iterator operator++(int);
		const T& operator*() const;

		// we like the default copy constructor and assignment operator

	private:
		Iterator(const SCell *curr);

		const SCell *m_Curr;
	};
};

static int nCount = 0;	// this is used as a random value

template <class T>
CSkipList<T>::Iterator::Iterator()
{
	m_Curr = NULL;
}

template <class T>
CSkipList<T>::Iterator& CSkipList<T>::Iterator::operator++()
{
	m_Curr = m_Curr->nexten[0];
	return *this;
}

template <class T>
CSkipList<T>::Iterator CSkipList<T>::Iterator::operator++(int)
{
	const SCell *prev = m_Curr;
	m_Curr = m_Curr->nexten[0];
	return Iterator(prev);
}

template <class T>
const T& CSkipList<T>::Iterator::operator*() const
{
	return m_Curr->content;
}

template <class T>
inline BOOL operator==(const CSkipList<T>::Iterator& j,
		const CSkipList<T>::Iterator& k)
{
	return (&*j == &*k);
}

template <class T>
inline BOOL operator!=(const CSkipList<T>::Iterator& j,
		const CSkipList<T>::Iterator& k)
{
	return (&*j != &*k);
}

template <class T>
CSkipList<T>::Iterator::Iterator(const SCell *curr)
{
	m_Curr = curr;
}

template <class T>
CSkipList<T>::CSkipList()
	: m_Root(&m_Root, MAX_DEGREE)
{
	m_Degree = 0;
}

template <class T>
CSkipList<T>::~CSkipList()
{
	Clear();
}

template <class T>
void CSkipList<T>::Clear()
{
	SCell *curr = m_Root.nexten[0];
	SCell *next;

	while (curr != &m_Root)
	{
		next = curr->nexten[0];
		delete curr;
		curr = next;
	}
}

template <class T>
BOOL CSkipList<T>::Insert(const T& t)
{
	SCell *update[MAX_DEGREE + 1];
	SCell *cell = &m_Root;

	for (int i = m_Degree; i >= 0; i--)
	{
		while (cell->nexten[i] != &m_Root
				&& (const T&) cell->nexten[i]->content < t)
		{
			cell = cell->nexten[i];
		}
		update[i] = cell;
	}
	cell = cell->nexten[0];

	// if the item is already there, don't insert it again
	if (cell != &m_Root && cell->content == t)
		return FALSE;

	int cell_degree = 0;

	nCount += 2;
	while (cell_degree < MAX_DEGREE)
	{
		if ((nCount & ((1 << (cell_degree + 1)) - 2)) != 0)
			break;
		cell_degree++;
	}

	if (cell_degree > m_Degree)
	{
		for (int i = m_Degree + 1; i <= cell_degree; i++)
		{
			update[i] = &m_Root;
		}
		m_Degree = cell_degree;
	}

	cell = new SCell(&m_Root, t, cell_degree);
	for (int i = 0; i <= cell_degree; i++)
	{
		cell->nexten[i] = update[i]->nexten[i];
		update[i]->nexten[i] = cell;
	}
	return TRUE;
}

template <class T>
BOOL CSkipList<T>::Remove(const T& t)
{
	SCell *update[MAX_DEGREE + 1];
	SCell *cell = &m_Root;

	for (int i = m_Degree; i >= 0; i--)
	{
		while (cell->nexten[i] != &m_Root && cell->nexten[i]->content < t)
		{
			cell = cell->nexten[i];
		}
		update[i] = cell;
	}
	cell = cell->nexten[0];
	if (cell == &m_Root || cell->content != t)
		return FALSE;

	for (int i = 0; i <= m_Degree; i++)
	{
		if (update[i]->nexten[i] != cell)
			break;
		update[i]->nexten[i] = cell->nexten[i];
	}
	delete cell;
	return TRUE;
}

template <class T>
CSkipList<T>::Iterator CSkipList<T>::Find(const T& t) const
{
	const SCell *cell = &m_Root;

	for (int i = m_Degree; i >= 0; i--)
	{
		while (cell->nexten[i] != &m_Root && cell->nexten[i]->content < t)
		{
			cell = cell->nexten[i];
		}
	}
	return Iterator(cell->nexten[0]);
}

template <class T>
CSkipList<T>::Iterator CSkipList<T>::Begin() const
{
	return Iterator(m_Root.nexten[0]);
}

template <class T>
CSkipList<T>::Iterator CSkipList<T>::End() const
{
	return Iterator(&m_Root);
}

template <class T>
CSkipList<T>::SCell::SCell(SCell *root, const T& t, int degree)
	: content(t)
{
	nexten = new SCell *[degree + 1];
	for (int i = 0; i <= degree; i++)
	{
		nexten[i] = root;
	}
}

template <class T>
CSkipList<T>::SCell::SCell(SCell *root, int degree)
{
	nexten = new SCell *[degree + 1];
	for (int i = 0; i <= degree; i++)
	{
		nexten[i] = root;
	}
}

template <class T>
CSkipList<T>::SCell::~SCell()
{
	delete[] nexten;
}

#endif // __INC_SKIPLIST_H__
