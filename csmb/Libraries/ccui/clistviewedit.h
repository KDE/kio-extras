/* Name: clistviewedit.h

   Description: This file is a part of the ccui library.

   Author: Philippe Bouchard

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


#ifndef CLISTVIEWEDIT_H
#define CLISTVIEWEDIT_H

#include <qlistview.h>
#include <qlineedit.h>


/**
    Editable list view.

    Important: You program must be compiled with Run-Time Type Information.
    
    This class can be used to edit all the columns in a list view.

    Since list views can be sorted, the order of the lines is not
    considered important, therefore all of the QListViewItems must
    be stored in an array in order to retrieve their altered
    columns values.

    The constructor accepts a pointer to a function that will
    return a pointer to a widget, which will be used to edit a
    specific entry. Since CListViewEdit must have a common way to
    read and write from those widget, the class CListViewEdit::Entry<>
    must be used to instanciate those widgets, from which pointers to
    those specific member functions will be given. This way, you are
    able to use other specialized widget, like IP edit entries,
    combo boxes, etc.

    The constructor also accepts a predicate that will determine if a
    specific QListViewItem has the right to modify the value of
    one of its columns. The default defines QListViewItem that are
    parents to be read-only (cannot be altered).

    An validator can also be passed to the constructor, that will
    decide if an entry is acceptable just before commiting changes.
    If not, the entry widget will be shown until the user enters
    a correct entry or presses the Escape key.

    The class CListViewEdit::LineEdit is an example of how you can
    define a widget specialized to handle listview editions.

    Example:
    <pre>
    class CSomeClass : public QObject
    {
      Q_OBJECT

    protected:
      class CListViewEdit * pcListViewEdit1;

      class QListViewItem * apcListViewItem[14];
      class QWidget       * apcEntryType[14];

      static bool predicate(QObject *, QListViewItem *, int);
      static bool validator(QObject *, QListViewItem *, int);
      static QWidget * entryType(QObject *, QListViewItem *, int);

    public:
      CSomeClass(class QWidget * = 0, char const * = 0);
      ...
    };

    CSomeClass::CSomeClass(class QWidget * a_pcWidget, char const * a_pzName): QObject(a_pcWidget, a_pzName)
    {
      this->pcListViewEdit1 = new class CListViewEdit(this, "pcListViewEdit1");

      this->apcEntryType[0] = new class CListViewEdit::Entry<QLineEdit>(& QLineEdit::text, & QLineEdit::setText, this->pcListViewEdit1, "apcEntryType[0]");

      this->pcListViewEdit1->pcOwner = this;
      this->pcListViewEdit1->pEntryType = & CHardwareEdit::entryType;
      this->pcListViewEdit1->pPredicate = & CHardwareEdit::predicate;

      this->pcListViewEdit1->addColumn(i18n(""));
      this->pcListViewEdit1->addColumn(i18n("Query"));
      this->pcListViewEdit1->addColumn(i18n("Response"));

      this->apcListViewItem[0] = new class QListViewItem(this->pcListViewEdit1, i18n("Init"));
      this->apcListViewItem[1] = new class QListViewItem(this->apcListViewItem[0], i18n("Busy"));
      this->apcListViewItem[2] = new class QListViewItem(this->apcListViewItem[0], i18n("Not connected"));
      ...
    }

    bool CSomeClass::predicate(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      class CSomeClass * pcSomeClass = dynamic_cast<CSomeClass *>(a_cOwner);

      if (a_pcListViewItem == pcSomeClass->apcListViewItem[1] || a_pcListViewItem == pcSomeClass->apcListViewItem[2])
      {
        switch (a_iColumn)
        {
        case 2:   return true;
        }
      }
      else if (a_pcListViewItem->childCount() == 0)
      {
        return true;
      }

      return false;
    }

    bool CSomeClass::validator(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      return true;
    }    

    QWidget * CSomeClass::entryType(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      class CSomeClass * pcSomeClass = dynamic_cast<CSomeClass *>(a_cOwner);

      return pcSomeClass->apcEntryType[0];
    }
    </pre>
*/

class CListViewEdit : public QListView
{
  Q_OBJECT

public:
  class LineEdit;
  template <typename T> class Entry;
  template <typename T> friend class Entry;


  /**
      Edition mode flag.

      This flag will determine if edition must be called explicitly or not.
  */

  bool bExplicit;


  /**
      Owner of given static functions.

      This pointer will be passed to pPredicate() and pEntryType() functions.
  */

  class QObject * pcOwner;


  /**
      Pointer to an entry type function.

      The function will be called to determine the widget used to edit an entry.
  */

  QWidget * (* pEntryType)(QObject *, QListViewItem *, int);

  /**
      Pointer to a validator function.

      The function will be called to validate an entry.
  */

  bool (* pValidator)(QObject *, QListViewItem *, int);

  /**
      Pointer to a predicate.

      The predicate will return true if a specific entry is editable.
  */

  bool (* pPredicate)(QObject *, QListViewItem *, int);


  CListViewEdit(class QWidget * = 0, char const * = 0, QObject * = 0, QWidget * (*)(QObject *, QListViewItem *, int) = 0, bool (*)(QObject *, QListViewItem *, int) = 0, bool (*)(QObject *, QListViewItem *, int) = & CListViewEdit::defaultPredicate, bool = false);

public slots:
  void cellSelected(QListViewItem *, int);

private:
  class EntryType;
  class EventFilter;
  friend class EventFilter;


  /**
      Object used to filter events of CListViewEdit & CListViewEdit::Entry<>.
  */

  class EventFilter   * pcEventFilter;

protected:
  /**
      Actual column in which an edition is occuring.
  */

  int iColumn;


  /**
      Actual widget displayed for current edition.
  */

  class QWidget       * pcEntry;


  /**
      Actual QListViewItem being edited.
  */

  class QListViewItem * pcListViewItem;

  static bool defaultPredicate(QObject *, QListViewItem *, int);
  
protected slots:
  virtual void cellSelected(QListViewItem *, const QPoint &, int);
  virtual bool cellDeselected(bool = true);
};


/**
    Select a cell.

    Called when a cell is explicitly requested for selection.

    @param  a_pcListViewItem          Requested QListViewItem
    @param  a_iColumn                 Requested column
*/

inline void CListViewEdit::cellSelected(QListViewItem * a_pcListViewItem, int iColumn)
{
  bool const bExplicit = this->bExplicit;
  this->bExplicit = false;
  this->cellSelected(a_pcListViewItem, QPoint(0, 0), iColumn);
  this->bExplicit = bExplicit;
}




/**
    Used to override events of other widgets.
*/

struct CListViewEdit::EventFilter : public QObject
{
  EventFilter(QWidget * = 0, char const * = 0);
  virtual bool eventFilter(QObject *, QEvent *);
};


/**
    Contructs an event filter object.

    @param  a_pcWidget                Parent widget
    @param  a_pzName                  Widget's name
*/

inline CListViewEdit::EventFilter::EventFilter(QWidget * a_pcWidget, char const * a_pzName): QObject(a_pcWidget, a_pzName)
{
}




/**
    Extra information on widgets

    This allows to give a common name to member functions
    that returns the current text.
*/

class CListViewEdit::EntryType
{
  friend class CListViewEdit;

  class QString (QWidget::* const pText)() const;
  void (QWidget::* const pSetText)(QString const &);

public:
  EntryType(QString (QWidget::* const)() const, void (QWidget::* const)(QString const &));
  virtual ~EntryType();
};


/**
    Contructs the base containing information
    on the widget's read & write member functions.

    Please note that there is a contravariance violation involved here,
    but since inheritance is not virtual, there should be no problem.

    @param  a_pText                   Pointer to member function that returns current text
    @param  a_pSetText                Pointer to member function that sets its text
*/

inline CListViewEdit::EntryType::EntryType(QString (QWidget::* const a_pText)() const, void (QWidget::* const a_pSetText)(QString const &)):pText(a_pText), pSetText(a_pSetText)
{
}


/**
    Destructor.
*/

inline CListViewEdit::EntryType::~EntryType()
{
}




/**
    Specialized version of a given QWidget type.

    This mainly links the widget with the given extra information.
*/

template <typename T> class CListViewEdit::Entry : public T, public CListViewEdit::EntryType
{
public:
  Entry(QString (T::* const)() const, void (T::* const)(QString const &), QWidget * = 0, char const * = 0);
};


/**
    Contructs a special version of a given widget.

    @param  a_pText                   Pointer to member function that returns current text
    @param  a_pSetText                Pointer to member function that sets its text
    @param  a_pcWidget                Parent widget
    @param  a_pzName                  Widget's name
*/

template <typename T> inline CListViewEdit::Entry<T>::Entry(QString (T::* const a_pText)() const, void (T::* const a_pSetText)(QString const &), QWidget * a_pcParent, char const * a_pzName): T(a_pcParent, a_pzName), EntryType((QString (QWidget::* const)() const) a_pText, (void (QWidget::* const)(QString const &)) a_pSetText)
{
  this->T::hide();
}




/**
    Specialized version of QLineEdit.

    Only selects all its text after setText() is called.
*/

class CListViewEdit::LineEdit : public QLineEdit
{
  Q_OBJECT

public:
  LineEdit(QWidget * a_pcWidget, char const * a_pzName);

public slots:
  virtual void setText(const QString &);
};


/**
    Constructs a specialized version of QLineEdit.

    @param  a_pcWidget                Parent widget
    @param  a_pzName                  Widget's name
*/

inline CListViewEdit::LineEdit::LineEdit(QWidget * a_pcWidget, char const * a_pzName): QLineEdit(a_pcWidget, a_pzName)
{
}


/**
    Sets the text and select it all.

    @param  a_cString                 New text
*/

inline void CListViewEdit::LineEdit::setText(const QString & a_cString)
{
  this->QLineEdit::setText(a_cString);
  this->QLineEdit::selectAll();
}


#endif
