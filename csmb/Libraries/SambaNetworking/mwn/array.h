/* Name: array.h

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

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


/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#ifndef __INC_ARRAY_H_
#define __INC_ARRAY_H_

inline void *operator new(size_t, void *p) { return p; }

template<class REF, class ARG> class CVector
{
public:
  CVector();
  int count() const;
  void resize(int nNewCount, int nStep = -1);
  void clear();
  
  void setResize(int n, ARG element);
  
  int Add(ARG element);
  REF operator[](int n) const;
  REF& operator[](int n);
  
  void InsertAt(int n, ARG element, int count = 1);
  void RemoveAt(int n, int count = 1);

protected:
  REF* data;
  int m_nCount;
  int m_nMaxCount;
  int m_nStep;

public:
  ~CVector();
};

template<class REF, class ARG> CVector<REF, ARG>::CVector()
{
  data = NULL;
  m_nCount = 0;
  m_nMaxCount = 0;
  m_nStep = 0;
}

template<class REF> inline void initArray(REF* p, int n)
{
  memset((void*)p, 0, n * sizeof(REF));

  for (; n--; p++)
    ::new((void*)p) REF;
}

template<class REF> inline void destroyArray(REF* p, int n)
{
  for (; n--; p++)
    p->~REF();
}

template<class REF, class ARG> CVector<REF, ARG>::~CVector()
{
  if (NULL != data)
  {
    destroyArray<REF>(data, m_nCount);
    delete[] (unsigned char*)data;
  }
}

template<class REF> inline void copyArray(REF* src, const REF* dst, int n)
{
  while (n--)
    *dst++ = *src++;
}

template<class REF, class ARG> inline int CVector<REF, ARG>::count() const
{
  return m_nCount;
}

template<class REF, class ARG> inline void CVector<REF, ARG>::clear()
{ 
  resize(0, -1);
}

template<class REF, class ARG> inline int CVector<REF, ARG>::Add(ARG item)
{ 
  int n = m_nCount;
  setResize(n, item);
  return n;
}

template<class REF, class ARG> inline REF CVector<REF, ARG>::operator[](int n) const
{
  return data[n];
}

template<class REF, class ARG> inline REF& CVector<REF, ARG>::operator[](int n)
{
  return data[n];
}

template<class REF, class ARG> void CVector<REF, ARG>::resize(int nNewCount, int nStep)
{
  if (nStep != -1)
    m_nStep = nStep;

  if (nNewCount == 0)
  {
    if (NULL != data)
    {
      destroyArray<REF>(data, m_nCount);
      delete[] (unsigned char*)data;
      data = NULL;
    }
    
    m_nCount = m_nMaxCount = 0;
  }
  else
  {
    if (NULL == data)
    {
      data = (REF*) new unsigned char[nNewCount * sizeof(REF)];
      initArray<REF>(data, nNewCount);
      m_nCount = m_nMaxCount = nNewCount;
    }
    else
    {
      if (nNewCount <= m_nMaxCount)
      {
        if (nNewCount > m_nCount)
    	    initArray<REF>(&data[m_nCount], nNewCount - m_nCount);
        else
        {
          if (m_nCount > nNewCount)
            destroyArray<REF>(&data[nNewCount], m_nCount - nNewCount);
        }
	
        m_nCount = nNewCount;
      }
      else
      {
        int nStep = m_nStep;
        
        if (!nStep)
        {
          nStep = m_nCount / 8;
          nStep = (nStep < 4) ? 4 : ((nStep > 1024) ? 1024 : nStep);
        }
		
        int nNewMax;
        
        if (nNewCount < m_nMaxCount + nStep)
          nNewMax = m_nMaxCount + nStep;
        else
          nNewMax = nNewCount;

    		REF* pNewData = (REF*) new unsigned char[nNewMax * sizeof(REF)];
    
    		memcpy(pNewData, data, m_nCount * sizeof(REF));
    
    		initArray<REF>(&pNewData[m_nCount], nNewCount - m_nCount);
    
    		delete[] (unsigned char*)data;
    		data = pNewData;
    		m_nCount = nNewCount;
    		m_nMaxCount = nNewMax;
    	}
    }
  }
}

template<class REF, class ARG> void CVector<REF, ARG>::setResize(int n, ARG element)
{
  if (n >= m_nCount)
    resize(n+1, -1);
    
  data[n] = element;
}

template<class REF, class ARG> void CVector<REF, ARG>::InsertAt(int n, ARG element, int count /*=1*/)
{
  if (n >= m_nCount)
  {
    resize(n + count, -1);
  }
  else
  {
    int oldsize = m_nCount;
    resize(m_nCount + count, -1);
    destroyArray<REF>(&data[oldsize], count);
    memmove(&data[n+count], &data[n], (oldsize-n) * sizeof(REF));
    initArray<REF>(&data[n], count);
  }

  while (count--)
    data[n++] = element;
}

template<class REF, class ARG> void CVector<REF, ARG>::RemoveAt(int n, int count)
{
  int nMoveCount = m_nCount - n - count;
  destroyArray<REF>(&data[n], count);

  if (nMoveCount)
    memcpy(&data[n], &data[n + count], nMoveCount * sizeof(REF));
    
  m_nCount -= count;
}

#endif /* __INC_ARRAY_H_ */

