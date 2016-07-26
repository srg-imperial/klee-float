//===-- Expr.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPR_H
#define KLEE_EXPR_H

#include "klee/util/Bits.h"
#include "klee/util/Ref.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>
#include <set>
#include <vector>
#include <map>

namespace llvm {
  class Type;
  class raw_ostream;
}

namespace klee {

class Array;
class ArrayCache;
class ConstantExpr;
class ObjectState;

template<class T> class ref;


/// Class representing symbolic expressions.
/**

<b>Expression canonicalization</b>: we define certain rules for
canonicalization rules for Exprs in order to simplify code that
pattern matches Exprs (since the number of forms are reduced), to open
up further chances for optimization, and to increase similarity for
caching and other purposes.

The general rules are:
<ol>
<li> No Expr has all constant arguments.</li>

<li> Booleans:
    <ol type="a">
     <li> \c Ne, \c Ugt, \c Uge, \c Sgt, \c Sge are not used </li>
     <li> The only acceptable operations with boolean arguments are 
          \c Not \c And, \c Or, \c Xor, \c Eq, 
	  as well as \c SExt, \c ZExt,
          \c Select and \c NotOptimized. </li>
     <li> The only boolean operation which may involve a constant is boolean not (<tt>== false</tt>). </li>
     </ol>
</li>

<li> Linear Formulas: 
   <ol type="a">
   <li> For any subtree representing a linear formula, a constant
   term must be on the LHS of the root node of the subtree.  In particular, 
   in a BinaryExpr a constant must always be on the LHS.  For example, subtraction 
   by a constant c is written as <tt>add(-c, ?)</tt>.  </li>
    </ol>
</li>


<li> Chains are unbalanced to the right </li>

</ol>


<b>Steps required for adding an expr</b>:
   -# Add case to printKind
   -# Add to ExprVisitor
   -# Add to IVC (implied value concretization) if possible

Todo: Shouldn't bool \c Xor just be written as not equal?

*/

class Expr {
public:
  static unsigned count;
  static const unsigned MAGIC_HASH_CONSTANT = 39;

  /// The type of an expression is simply its width, in bits. 
  typedef unsigned Width; 
  
  static const Width InvalidWidth = 0;
  static const Width Bool = 1;
  static const Width Int8 = 8;
  static const Width Int16 = 16;
  static const Width Int32 = 32;
  static const Width Int64 = 64;
  static const Width Fl32 = 32;
  static const Width Fl64 = 64;
  static const Width Fl80 = 80;
  

  enum Type {
    Integer,
    FloatingPoint,
    RawBits
  };

  enum Kind {
    InvalidKind = -1,

    // Primitive

    Constant = 0,

    // Special

    /// Prevents optimization below the given expression.  Used for
    /// testing: make equality constraints that KLEE will not use to
    /// optimize to concretes.
    NotOptimized,

    //// Skip old varexpr, just for deserialization, purge at some point
    Read=NotOptimized+2, 
    Select,
    Concat,
    Extract,

    Int,

    // Casting,
    ZExt,
    SExt,
    FToU,
    FToS,

    // Bit
    Not,

    // Floating-point special functions
    FpClassify,
    FIsFinite,
    FIsNan,
    FIsInf,

    // Arithmetic
    Add,
    Sub,
    Mul,
    UDiv,
    SDiv,
    URem,
    SRem,

    // Bit
    And,
    Or,
    Xor,
    Shl,
    LShr,
    AShr,

    // Compare
    Eq,
    Ne,  ///< Not used in canonical form
    Ult,
    Ule,
    Ugt, ///< Not used in canonical form
    Uge, ///< Not used in canonical form
    Slt,
    Sle,
    Sgt, ///< Not used in canonical form
    Sge, ///< Not used in canonical form

    // Compare floating-point
    FOrd,
    FUno,
    FUeq,
    FOeq,
    FUgt,
    FOgt,
    FUge,
    FOge,
    FUlt,
    FOlt,
    FUle,
    FOle,
    FUne,
    FOne,

    // Float
    Float,

    FloatConstant,

    // Special
    FloatSelect,

    // Casting,
    FExt,
    UToF,
    SToF,

    // Special functions
    FAbs,
    FSqrt,
    FNearbyInt,

    // Arithmetic
    FAdd,
    FSub,
    FMul,
    FDiv,
    
    // Special functions
    FRem,
    FMin,
    FMax,

    LastKind=FMax,

    CastKindFirst=ZExt,
    CastKindLast=FToS,
    CastRoundKindFirst=FToU,
    CastRoundKindLast=FToS,
    UnaryKindFirst=FpClassify,
    UnaryKindLast=FIsInf,
    BinaryKindFirst=Add,
    BinaryKindLast=FOne,
    CmpKindFirst=Eq,
    CmpKindLast=FOne,
    FloatCastKindFirst=FExt,
    FloatCastKindLast=SToF,
    FloatCastRoundKindFirst=FToU,
    FloatCastRoundKindLast=FToS,
    FloatUnaryKindFirst=FAbs,
    FloatUnaryKindLast=FNearbyInt,
    FloatUnaryRoundKindFirst=FSqrt,
    FloatUnaryRoundKindLast=FNearbyInt,
    FloatBinaryKindFirst=FAdd,
    FloatBinaryKindLast=FMax,
    FloatBinaryRoundKindFirst=FAdd,
    FloatBinaryRoundKindLast=FRem
  };

  unsigned refCount;

protected:  
  unsigned hashValue;

  /// Compares `b` to `this` Expr and determines how they are ordered
  /// (ignoring their kid expressions - i.e. those returned by `getKid()`).
  ///
  /// Typically this requires comparing internal attributes of the Expr.
  ///
  /// Implementations can assume that `b` and `this` are of the same kind.
  ///
  /// This method effectively defines a partial order over Expr of the same
  /// kind (partial because kid Expr are not compared).
  ///
  /// This method should not be called directly. Instead `compare()` should
  /// be used.
  ///
  /// \param [in] b Expr to compare `this` to.
  ///
  /// \return One of the following values:
  ///
  /// * -1 if `this` is `<` `b` ignoring kid expressions.
  /// * 1 if `this` is `>` `b` ignoring kid expressions.
  /// * 0 if `this` and `b` are not ordered.
  ///
  /// `<` and `>` are binary relations that express the partial order.
  virtual int compareContents(const Expr &b) const = 0;

public:
  Expr() : refCount(0) { Expr::count++; }
  virtual ~Expr() { Expr::count--; } 

  virtual Kind getKind() const = 0;
  virtual Width getWidth() const = 0;
  virtual Type getType() const = 0;
  
  virtual unsigned getNumKids() const = 0;
  virtual ref<Expr> getKid(unsigned i) const = 0;
    
  virtual void print(llvm::raw_ostream &os) const;

  /// dump - Print the expression to stderr.
  void dump() const;

  /// Returns the pre-computed hash of the current expression
  virtual unsigned hash() const { return hashValue; }

  /// (Re)computes the hash of the current expression.
  /// Returns the hash value. 
  virtual unsigned computeHash();
  
  /// Compares `b` to `this` Expr for structural equivalence.
  ///
  /// This method effectively defines a total order over all Expr.
  ///
  /// \param [in] b Expr to compare `this` to.
  ///
  /// \return One of the following values:
  ///
  /// * -1 iff `this` is `<` `b`
  /// * 0 iff `this` is structurally equivalent to `b`
  /// * 1 iff `this` is `>` `b`
  ///
  /// `<` and `>` are binary relations that express the total order.
  int compare(const Expr &b) const;

  // Given an array of new kids return a copy of the expression
  // but using those children. 
  virtual ref<Expr> rebuild(ref<Expr> kids[/* getNumKids() */]) const = 0;

  /// isZero - Is this a constant zero.
  bool isZero() const;
  
  /// isTrue - Is this the true expression.
  bool isTrue() const;

  /// isFalse - Is this the false expression.
  bool isFalse() const;

  /* Static utility methods */

  static void printKind(llvm::raw_ostream &os, Kind k);
  static void printWidth(llvm::raw_ostream &os, Expr::Width w);
  static void printType(llvm::raw_ostream & os, Expr::Type t);

  /// returns the smallest number of bytes in which the given width fits
  static inline unsigned getMinBytesForWidth(Width w) {
      return (w + 7) / 8;
  }

  /* Kind utilities */

  /* Utility creation functions */
  static ref<Expr> createSExtToPointerWidth(ref<Expr> e);
  static ref<Expr> createZExtToPointerWidth(ref<Expr> e);
  static ref<Expr> createImplies(ref<Expr> hyp, ref<Expr> conc);
  static ref<Expr> createIsZero(ref<Expr> e);

  /// Create a little endian read of the given type at offset 0 of the
  /// given object.
  static ref<Expr> createTempRead(const Array *array, Expr::Width w);
  
  static ref<ConstantExpr> createPointer(uint64_t v);

  struct CreateArg;
  static ref<Expr> createFromKind(Kind k, std::vector<CreateArg> args);

  static bool isValidKidWidth(unsigned kid, Width w) { return true; }
  static bool needsResultType() { return false; }

  static bool classof(const Expr *) { return true; }

private:
  typedef llvm::DenseSet<std::pair<const Expr *, const Expr *> > ExprEquivSet;
  int compare(const Expr &b, ExprEquivSet &equivs) const;
};

struct Expr::CreateArg {
  ref<Expr> expr;
  Width width;
  llvm::APFloat::roundingMode rm;
  
  CreateArg(Width w = Bool) : expr(0), width(w), rm(llvm::APFloat::rmNearestTiesToEven), isRM(false) {}
  CreateArg(ref<Expr> e) : expr(e), width(Expr::InvalidWidth), rm(llvm::APFloat::rmNearestTiesToEven), isRM(false) {}
  CreateArg(llvm::APFloat::roundingMode r) : expr(0), width(Expr::InvalidWidth), rm(r), isRM(true) {}
  
  bool isWidth() { return width != Expr::InvalidWidth; }
  bool isRoundingMode() { return isRM; }
  bool isExpr() { return !isWidth() && !isRoundingMode(); }

private:
  bool isRM;
};

// Comparison operators

inline bool operator==(const Expr &lhs, const Expr &rhs) {
  return lhs.compare(rhs) == 0;
}

inline bool operator<(const Expr &lhs, const Expr &rhs) {
  return lhs.compare(rhs) < 0;
}

inline bool operator!=(const Expr &lhs, const Expr &rhs) {
  return !(lhs == rhs);
}

inline bool operator>(const Expr &lhs, const Expr &rhs) {
  return rhs < lhs;
}

inline bool operator<=(const Expr &lhs, const Expr &rhs) {
  return !(lhs > rhs);
}

inline bool operator>=(const Expr &lhs, const Expr &rhs) {
  return !(lhs < rhs);
}

// Printing operators

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Expr &e) {
  e.print(os);
  return os;
}

// XXX the following macro is to work around the ExprTest unit test compile error
#ifndef LLVM_29_UNITTEST
inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Expr::Kind kind) {
  Expr::printKind(os, kind);
  return os;
}
#endif

inline std::stringstream &operator<<(std::stringstream &os, const Expr &e) {
  std::string str;
  llvm::raw_string_ostream TmpStr(str);
  e.print(TmpStr);
  os << TmpStr.str();
  return os;
}

inline std::stringstream &operator<<(std::stringstream &os, const Expr::Kind kind) {
  std::string str;
  llvm::raw_string_ostream TmpStr(str);
  Expr::printKind(TmpStr, kind);
  os << TmpStr.str();
  return os;
}

// Utility classes

class IntExpr : public Expr {
public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return k >= Expr::Constant && k < Expr::Float;
  }
  static bool classof(const IntExpr *) { return true; }
};

class NonConstantExpr : public IntExpr {
public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return k != Expr::Constant && k < Expr::Float;
  }
  static bool classof(const NonConstantExpr *) { return true; }
};

class BinaryExpr : public NonConstantExpr {
public:
  ref<Expr> left, right;

public:
  unsigned getNumKids() const { return 2; }
  ref<Expr> getKid(unsigned i) const { 
    if(i == 0)
      return left;
    if(i == 1)
      return right;
    return 0;
  }
 
protected:
  BinaryExpr(const ref<Expr> &l, const ref<Expr> &r) : left(l), right(r) {}

public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::BinaryKindFirst <= k && k <= Expr::BinaryKindLast;
  }
  static bool classof(const BinaryExpr *) { return true; }
};


class CmpExpr : public BinaryExpr {

protected:
  CmpExpr(ref<Expr> l, ref<Expr> r) : BinaryExpr(l,r) {}
  
public:                                                       
  Width getWidth() const { return Bool; }
  Type getType() const { return Expr::Integer; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::CmpKindFirst <= k && k <= Expr::CmpKindLast;
  }
  static bool classof(const CmpExpr *) { return true; }
};

// Special

class NotOptimizedExpr : public NonConstantExpr {
public:
  static const Kind kind = NotOptimized;
  static const unsigned numKids = 1;
  ref<Expr> src;

  static ref<Expr> alloc(const ref<Expr> &src) {
    ref<Expr> r(new NotOptimizedExpr(src));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(ref<Expr> src);
  
  Width getWidth() const { return src->getWidth(); }
  Type getType() const { return src->getType(); }
  Kind getKind() const { return NotOptimized; }

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return src; }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { return create(kids[0]); }

private:
  NotOptimizedExpr(const ref<Expr> &_src) : src(_src) {}

protected:
  virtual int compareContents(const Expr &b) const {
    // No attributes to compare.
    return 0;
  }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::NotOptimized;
  }
  static bool classof(const NotOptimizedExpr *) { return true; }
};


/// Class representing a byte update of an array.
class UpdateNode {
  friend class UpdateList;  

  mutable unsigned refCount;
  // cache instead of recalc
  unsigned hashValue;

public:
  const UpdateNode *next;
  ref<Expr> index, value;
  
private:
  /// size of this update sequence, including this update
  unsigned size;
  
public:
  UpdateNode(const UpdateNode *_next, 
             const ref<Expr> &_index, 
             const ref<Expr> &_value);

  unsigned getSize() const { return size; }

  int compare(const UpdateNode &b) const;  
  unsigned hash() const { return hashValue; }

private:
  UpdateNode() : refCount(0) {}
  ~UpdateNode();

  unsigned computeHash();
};

class Array {
public:
  // Name of the array
  const std::string name;

  // FIXME: Not 64-bit clean.
  const unsigned size;

  /// Domain is how many bits can be used to access the array [32 bits]
  /// Range is the size (in bits) of the number stored there (array of bytes -> 8)
  const Expr::Width domain, range;

  /// constantValues - The constant initial values for this array, or empty for
  /// a symbolic array. If non-empty, this size of this array is equivalent to
  /// the array size.
  const std::vector<ref<ConstantExpr> > constantValues;

private:
  unsigned hashValue;

  // FIXME: Make =delete when we switch to C++11
  Array(const Array& array);

  // FIXME: Make =delete when we switch to C++11
  Array& operator =(const Array& array);

  ~Array();

  /// Array - Construct a new array object.
  ///
  /// \param _name - The name for this array. Names should generally be unique
  /// across an application, but this is not necessary for correctness, except
  /// when printing expressions. When expressions are printed the output will
  /// not parse correctly since two arrays with the same name cannot be
  /// distinguished once printed.
  Array(const std::string &_name, uint64_t _size,
        const ref<ConstantExpr> *constantValuesBegin = 0,
        const ref<ConstantExpr> *constantValuesEnd = 0,
        Expr::Width _domain = Expr::Int32, Expr::Width _range = Expr::Int8);

public:
  bool isSymbolicArray() const { return constantValues.empty(); }
  bool isConstantArray() const { return !isSymbolicArray(); }

  const std::string getName() const { return name; }
  unsigned getSize() const { return size; }
  Expr::Width getDomain() const { return domain; }
  Expr::Width getRange() const { return range; }

  /// ComputeHash must take into account the name, the size, the domain, and the range
  unsigned computeHash();
  unsigned hash() const { return hashValue; }
  friend class ArrayCache;
};

/// Class representing a complete list of updates into an array.
class UpdateList { 
  friend class ReadExpr; // for default constructor

public:
  const Array *root;
  
  /// pointer to the most recent update node
  const UpdateNode *head;
  
public:
  UpdateList(const Array *_root, const UpdateNode *_head);
  UpdateList(const UpdateList &b);
  ~UpdateList();
  
  UpdateList &operator=(const UpdateList &b);

  /// size of this update list
  unsigned getSize() const { return (head ? head->getSize() : 0); }
  
  void extend(const ref<Expr> &index, const ref<Expr> &value);

  int compare(const UpdateList &b) const;
  unsigned hash() const;
private:
  void tryFreeNodes();
};

/// Class representing a one byte read from an array. 
class ReadExpr : public NonConstantExpr {
public:
  static const Kind kind = Read;
  static const unsigned numKids = 1;
  
public:
  UpdateList updates;
  ref<Expr> index;

public:
  static ref<Expr> alloc(const UpdateList &updates, const ref<Expr> &index) {
    ref<Expr> r(new ReadExpr(updates, index));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(const UpdateList &updates, ref<Expr> i);
  
  Width getWidth() const { assert(updates.root); return updates.root->getRange(); }
  Type getType() const { return RawBits; }
  Kind getKind() const { return Read; }
  
  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return !i ? index : 0; }
  
  int compareContents(const Expr &b) const;

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const {
    return create(updates, kids[0]);
  }

  virtual unsigned computeHash();

private:
  ReadExpr(const UpdateList &_updates, const ref<Expr> &_index) : 
    updates(_updates), index(_index) { assert(updates.root); }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Read;
  }
  static bool classof(const ReadExpr *) { return true; }
};


/// Class representing an if-then-else expression.
class SelectExpr : public NonConstantExpr {
public:
  static const Kind kind = Select;
  static const unsigned numKids = 3;
  
public:
  ref<Expr> cond, trueExpr, falseExpr;

public:
  static ref<Expr> alloc(const ref<Expr> &c, const ref<Expr> &t, 
                         const ref<Expr> &f) {
    ref<Expr> r(new SelectExpr(c, t, f));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(ref<Expr> c, ref<Expr> t, ref<Expr> f);

  Width getWidth() const { return trueExpr->getWidth(); }
  Type getType() const { assert(trueExpr->getType() == falseExpr->getType()); return trueExpr->getType(); }
  Kind getKind() const { return Select; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { 
        switch(i) {
        case 0: return cond;
        case 1: return trueExpr;
        case 2: return falseExpr;
        default: return 0;
        }
   }

  static bool isValidKidWidth(unsigned kid, Width w) {
    if (kid == 0)
      return w == Bool;
    else
      return true;
  }
    
  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0], kids[1], kids[2]);
  }

private:
  SelectExpr(const ref<Expr> &c, const ref<Expr> &t, const ref<Expr> &f) 
    : cond(c), trueExpr(t), falseExpr(f) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Select;
  }
  static bool classof(const SelectExpr *) { return true; }

protected:
  virtual int compareContents(const Expr &b) const {
    // No attributes to compare.
    return 0;
  }
};


/** Children of a concat expression can have arbitrary widths.  
    Kid 0 is the left kid, kid 1 is the right kid.
*/
class ConcatExpr : public NonConstantExpr { 
public: 
  static const Kind kind = Concat;
  static const unsigned numKids = 2;

private:
  Width width;
  ref<Expr> left, right;  

public:
  static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) {
    ref<Expr> c(new ConcatExpr(l, r));
    c->computeHash();
    return c;
  }
  
  static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r);

  Width getWidth() const { return width; }
  // TODO: look into setting this to float or int in the constructor
  Type getType() const { return RawBits; }
  Kind getKind() const { return kind; }
  ref<Expr> getLeft() const { return left; }
  ref<Expr> getRight() const { return right; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { 
    if (i == 0) return left; 
    else if (i == 1) return right;
    else return NULL;
  }

  /// Shortcuts to create larger concats.  The chain returned is unbalanced to the right
  static ref<Expr> createN(unsigned nKids, const ref<Expr> kids[]);
  static ref<Expr> create4(const ref<Expr> &kid1, const ref<Expr> &kid2,
			   const ref<Expr> &kid3, const ref<Expr> &kid4);
  static ref<Expr> create8(const ref<Expr> &kid1, const ref<Expr> &kid2,
			   const ref<Expr> &kid3, const ref<Expr> &kid4,
			   const ref<Expr> &kid5, const ref<Expr> &kid6,
			   const ref<Expr> &kid7, const ref<Expr> &kid8);
  
  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { return create(kids[0], kids[1]); }
  
private:
  ConcatExpr(const ref<Expr> &l, const ref<Expr> &r) : left(l), right(r) {
    width = l->getWidth() + r->getWidth();
  }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Concat;
  }
  static bool classof(const ConcatExpr *) { return true; }

protected:
  virtual int compareContents(const Expr &b) const {
    const ConcatExpr &eb = static_cast<const ConcatExpr &>(b);
    if (width != eb.width)
      return width < eb.width ? -1 : 1;
    return 0;
  }
};


/** This class represents an extract from expression {\tt expr}, at
    bit offset {\tt offset} of width {\tt width}.  Bit 0 is the right most 
    bit of the expression.
 */
class ExtractExpr : public NonConstantExpr { 
public:
  static const Kind kind = Extract;
  static const unsigned numKids = 1;
  
public:
  ref<Expr> expr;
  unsigned offset;
  Width width;

public:  
  static ref<Expr> alloc(const ref<Expr> &e, unsigned o, Width w) {
    ref<Expr> r(new ExtractExpr(e, o, w));
    r->computeHash();
    return r;
  }
  
  /// Creates an ExtractExpr with the given bit offset and width
  static ref<Expr> create(ref<Expr> e, unsigned bitOff, Width w);

  Width getWidth() const { return width; }
  Type getType() const { return expr->getType(); }
  Kind getKind() const { return Extract; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return expr; }

  int compareContents(const Expr &b) const {
    const ExtractExpr &eb = static_cast<const ExtractExpr&>(b);
    if (offset != eb.offset) return offset < eb.offset ? -1 : 1;
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0], offset, width);
  }

  virtual unsigned computeHash();

private:
  ExtractExpr(const ref<Expr> &e, unsigned b, Width w) 
    : expr(e),offset(b),width(w) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Extract;
  }
  static bool classof(const ExtractExpr *) { return true; }
};


/** 
    Bitwise Not 
*/
class NotExpr : public NonConstantExpr { 
public:
  static const Kind kind = Not;
  static const unsigned numKids = 1;
  
  ref<Expr> expr;

public:  
  static ref<Expr> alloc(const ref<Expr> &e) {
    ref<Expr> r(new NotExpr(e));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(const ref<Expr> &e);

  Width getWidth() const { return expr->getWidth(); }
  Type getType() const { return expr->getType(); }
  Kind getKind() const { return Not; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return expr; }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0]);
  }

  virtual unsigned computeHash();

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Not;
  }
  static bool classof(const NotExpr *) { return true; }

private:
  NotExpr(const ref<Expr> &e) : expr(e) {}

protected:
  virtual int compareContents(const Expr &b) const {
    // No attributes to compare.
    return 0;
  }
};



// Casting

class CastExpr : public NonConstantExpr {
public:
  ref<Expr> src;
  Width width;

public:
  CastExpr(const ref<Expr> &e, Width w) : src(e), width(w) {}

  Width getWidth() const { return width; }

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return (i==0) ? src : 0; }
  
  static bool needsResultType() { return true; }
  
  int compareContents(const Expr &b) const {
    const CastExpr &eb = static_cast<const CastExpr&>(b);
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  virtual unsigned computeHash();

  static bool classof(const Expr *E) {
    Expr::Kind k = E->getKind();
    return Expr::CastKindFirst <= k && k <= Expr::CastKindLast;
  }
  static bool classof(const CastExpr *) { return true; }
};

#define CAST_EXPR_CLASS(_class_kind)                             \
class _class_kind ## Expr : public CastExpr {                    \
public:                                                          \
  static const Kind kind = _class_kind;                          \
  static const unsigned numKids = 1;                             \
public:                                                          \
    _class_kind ## Expr(ref<Expr> e, Width w) : CastExpr(e,w) {} \
    static ref<Expr> alloc(const ref<Expr> &e, Width w) {        \
      ref<Expr> r(new _class_kind ## Expr(e, w));                \
      r->computeHash();                                          \
      return r;                                                  \
    }                                                            \
    static ref<Expr> create(const ref<Expr> &e, Width w);        \
    Kind getKind() const { return _class_kind; }                 \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {          \
      return create(kids[0], width);                             \
    }                                                            \
                                                                 \
    Type getType() const { return Expr::Integer; }               \
                                                                 \
    static bool classof(const Expr *E) {                         \
      return E->getKind() == Expr::_class_kind;                  \
    }                                                            \
    static bool classof(const  _class_kind ## Expr *) {          \
      return true;                                               \
    }                                                            \
};                                                               \

CAST_EXPR_CLASS(SExt)
CAST_EXPR_CLASS(ZExt)

// Casting with roundingMode

class CastRoundExpr : public CastExpr {
public:
  llvm::APFloat::roundingMode round;

public:
  CastRoundExpr(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm) : CastExpr(e,w), round(rm) {}

  llvm::APFloat::roundingMode getRoundingMode() const { return round; }

  int compareContents(const Expr &b) const {
    const CastRoundExpr &eb = static_cast<const CastRoundExpr&>(b);
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  static bool classof(const Expr *E) {
    Expr::Kind k = E->getKind();
    return Expr::CastRoundKindFirst <= k && k <= Expr::CastRoundKindLast;
  }
  static bool classof(const CastRoundExpr *) { return true; }
};

#define CAST_RM_EXPR_CLASS(_class_kind)                                \
class _class_kind ## Expr : public CastRoundExpr {                                \
public:                                                                           \
  static const Kind kind = _class_kind;                                           \
  static const unsigned numKids = 1;                                              \
  static const llvm::APFloat::roundingMode round = llvm::APFloat::rmNearestTiesToEven;                          \
public:                                                                           \
    _class_kind ## Expr(ref<Expr> e, Width w, llvm::APFloat::roundingMode rm) : CastRoundExpr(e,w,rm) {} \
    static ref<Expr> alloc(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm) {        \
      ref<Expr> r(new _class_kind ## Expr(e, w, rm));                             \
      r->computeHash();                                                           \
      return r;                                                                   \
    }                                                                             \
    static ref<Expr> create(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm);        \
    Kind getKind() const { return _class_kind; }                                  \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                           \
      return create(kids[0], width, round);                                       \
    }                                                                             \
                                                                                  \
    Type getType() const { return Expr::Integer; }                                    \
                                                                                  \
    static bool classof(const Expr *E) {                                          \
      return E->getKind() == Expr::_class_kind;                                   \
    }                                                                             \
    static bool classof(const  _class_kind ## Expr *) {                           \
      return true;                                                                \
    }                                                                             \
};                                                                                \

CAST_RM_EXPR_CLASS(FToU)
CAST_RM_EXPR_CLASS(FToS)

// Floating-Point unary special functions
class UnaryExpr : public NonConstantExpr {
public:
  ref<Expr> expr;

public:
  UnaryExpr(const ref<Expr> &e) : expr(e) {}

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return (i == 0) ? expr : 0; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::UnaryKindFirst <= k && k <= Expr::UnaryKindLast;
  }
  static bool classof(const UnaryExpr *) { return true; }
};

#define UNARY_EXPR_CLASS(_class_kind)               \
class _class_kind ## Expr : public UnaryExpr {          \
public:                                                 \
  static const Kind kind = _class_kind;                 \
  static const unsigned numKids = 1;                    \
public:                                                 \
    _class_kind ## Expr(ref<Expr> e) : UnaryExpr(e) {}  \
    static ref<Expr> alloc(const ref<Expr> &e) {        \
      ref<Expr> r(new _class_kind ## Expr(e));          \
      r->computeHash();                                 \
      return r;                                         \
    }                                                   \
    static ref<Expr> create(const ref<Expr> &e);        \
    Width getWidth() const { return sizeof(int) * 8; }  \
    Kind getKind() const { return _class_kind; }        \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const { \
      return create(kids[0]);                           \
    }                                                   \
                                                        \
    Type getType() const { return Expr::Integer; }      \
                                                        \
    static bool classof(const Expr *E) {                \
      return E->getKind() == Expr::_class_kind;         \
    }                                                   \
    static bool classof(const  _class_kind ## Expr *) { \
      return true;                                      \
    }                                                   \
};

UNARY_EXPR_CLASS(FpClassify)
UNARY_EXPR_CLASS(FIsFinite)
UNARY_EXPR_CLASS(FIsNan)
UNARY_EXPR_CLASS(FIsInf)

// Arithmetic/Bit Exprs

#define ARITHMETIC_EXPR_CLASS(_class_kind)                                     \
  class _class_kind##Expr : public BinaryExpr {                                \
  public:                                                                      \
    static const Kind kind = _class_kind;                                      \
    static const unsigned numKids = 2;                                         \
                                                                               \
  public:                                                                      \
    _class_kind##Expr(const ref<Expr> &l, const ref<Expr> &r)                  \
        : BinaryExpr(l, r) {}                                                  \
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) {           \
      ref<Expr> res(new _class_kind##Expr(l, r));                              \
      res->computeHash();                                                      \
      return res;                                                              \
    }                                                                          \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r);           \
    Width getWidth() const { return left->getWidth(); }                        \
    Kind getKind() const { return _class_kind; }                               \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                        \
      return create(kids[0], kids[1]);                                         \
    }                                                                          \
                                                                               \
    Type getType() const { return Expr::Integer; }                             \
                                                                               \
    static bool classof(const Expr *E) {                                       \
      return E->getKind() == Expr::_class_kind;                                \
    }                                                                          \
    static bool classof(const _class_kind##Expr *) { return true; }            \
                                                                               \
  protected:                                                                   \
    virtual int compareContents(const Expr &b) const {                         \
      /* No attributes to compare.*/                                           \
      return 0;                                                                \
    }                                                                          \
  };

ARITHMETIC_EXPR_CLASS(Add)
ARITHMETIC_EXPR_CLASS(Sub)
ARITHMETIC_EXPR_CLASS(Mul)
ARITHMETIC_EXPR_CLASS(UDiv)
ARITHMETIC_EXPR_CLASS(SDiv)
ARITHMETIC_EXPR_CLASS(URem)
ARITHMETIC_EXPR_CLASS(SRem)
ARITHMETIC_EXPR_CLASS(And)
ARITHMETIC_EXPR_CLASS(Or)
ARITHMETIC_EXPR_CLASS(Xor)
ARITHMETIC_EXPR_CLASS(Shl)
ARITHMETIC_EXPR_CLASS(LShr)
ARITHMETIC_EXPR_CLASS(AShr)

// Comparison Exprs

#define COMPARISON_EXPR_CLASS(_class_kind)                                     \
  class _class_kind##Expr : public CmpExpr {                                   \
  public:                                                                      \
    static const Kind kind = _class_kind;                                      \
    static const unsigned numKids = 2;                                         \
                                                                               \
  public:                                                                      \
    _class_kind##Expr(const ref<Expr> &l, const ref<Expr> &r)                  \
        : CmpExpr(l, r) {}                                                     \
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) {           \
      ref<Expr> res(new _class_kind##Expr(l, r));                              \
      res->computeHash();                                                      \
      return res;                                                              \
    }                                                                          \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r);           \
    Kind getKind() const { return _class_kind; }                               \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                        \
      return create(kids[0], kids[1]);                                         \
    }                                                                          \
                                                                               \
    Type getType() const { return Expr::Integer; }                             \
                                                                               \
    static bool classof(const Expr *E) {                                       \
      return E->getKind() == Expr::_class_kind;                                \
    }                                                                          \
    static bool classof(const _class_kind##Expr *) { return true; }            \
                                                                               \
  protected:                                                                   \
    virtual int compareContents(const Expr &b) const {                         \
      /* No attributes to compare. */                                          \
      return 0;                                                                \
    }                                                                          \
  };

COMPARISON_EXPR_CLASS(Eq)
COMPARISON_EXPR_CLASS(Ne)
COMPARISON_EXPR_CLASS(Ult)
COMPARISON_EXPR_CLASS(Ule)
COMPARISON_EXPR_CLASS(Ugt)
COMPARISON_EXPR_CLASS(Uge)
COMPARISON_EXPR_CLASS(Slt)
COMPARISON_EXPR_CLASS(Sle)
COMPARISON_EXPR_CLASS(Sgt)
COMPARISON_EXPR_CLASS(Sge)
COMPARISON_EXPR_CLASS(FOrd)
COMPARISON_EXPR_CLASS(FUno)
COMPARISON_EXPR_CLASS(FUeq)
COMPARISON_EXPR_CLASS(FOeq)
COMPARISON_EXPR_CLASS(FUgt)
COMPARISON_EXPR_CLASS(FOgt)
COMPARISON_EXPR_CLASS(FUge)
COMPARISON_EXPR_CLASS(FOge)
COMPARISON_EXPR_CLASS(FUlt)
COMPARISON_EXPR_CLASS(FOlt)
COMPARISON_EXPR_CLASS(FUle)
COMPARISON_EXPR_CLASS(FOle)
COMPARISON_EXPR_CLASS(FUne)
COMPARISON_EXPR_CLASS(FOne)

// Terminal Exprs

class ConstantExpr : public Expr {
public:
  static const Kind kind = Constant;
  static const unsigned numKids = 0;

private:
  llvm::APInt value;

  Type type;

  ConstantExpr(const llvm::APInt &v, Expr::Type t) : value(v), type(t) {}

public:
  ~ConstantExpr() {}

  Width getWidth() const { return value.getBitWidth(); }
  Type getType() const { return type; }
  Kind getKind() const { return Constant; }

  unsigned getNumKids() const { return 0; }
  ref<Expr> getKid(unsigned i) const { return 0; }

  /// getAPValue - Return the arbitrary precision value directly.
  ///
  /// Clients should generally not use the APInt value directly and instead use
  /// native ConstantExpr APIs.
  const llvm::APInt &getAPValue() const { return value; }

  /// getZExtValue - Returns the constant value zero extended to the
  /// return type of this method.
  ///
  ///\param bits - optional parameter that can be used to check that the
  /// number of bits used by this constant is <= to the parameter
  /// value. This is useful for checking that type casts won't truncate
  /// useful bits.
  ///
  /// Example: unit8_t byte= (unit8_t) constant->getZExtValue(8);
  uint64_t getZExtValue(unsigned bits = 64) const {
    assert(getWidth() <= bits && "Value may be out of range!");
    return value.getZExtValue();
  }

  /// getLimitedValue - If this value is smaller than the specified limit,
  /// return it, otherwise return the limit value.
  uint64_t getLimitedValue(uint64_t Limit = ~0ULL) const {
    return value.getLimitedValue(Limit);
  }

  /// toString - Return the constant value as a string
  /// \param Res specifies the string for the result to be placed in
  /// \param radix specifies the base (e.g. 2,10,16). The default is base 10
  void toString(std::string &Res, unsigned radix = 10) const;

  int compareContents(const Expr &b) const {
    const ConstantExpr &cb = static_cast<const ConstantExpr &>(b);
    if (getWidth() != cb.getWidth())
      return getWidth() < cb.getWidth() ? -1 : 1;
    if (value == cb.value)
      return 0;
    return value.ult(cb.value) ? -1 : 1;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const {
    assert(0 && "rebuild() on ConstantExpr");
    return const_cast<ConstantExpr *>(this);
  }

  virtual unsigned computeHash();

  static ref<Expr> fromMemory(void *address, Width w);
  void toMemory(void *address);

  static ref<ConstantExpr> alloc(const llvm::APInt &v, Expr::Type t) {
    ref<ConstantExpr> r(new ConstantExpr(v, t));
    r->computeHash();
    return r;
  }

  static ref<ConstantExpr> alloc(const llvm::APInt &v) {
    return alloc(v, Integer);
  }

  static ref<ConstantExpr> alloc(const llvm::APFloat &f) {
    return alloc(f.bitcastToAPInt(), FloatingPoint);
  }

  static ref<ConstantExpr> alloc(uint64_t v, Width w) {
    return alloc(llvm::APInt(w, v));
  }

  static ref<ConstantExpr> create(uint64_t v, Width w) {
#ifndef NDEBUG
    if (w <= 64)
      assert(v == bits64::truncateToNBits(v, w) && "invalid constant");
#endif
    return alloc(v, w);
  }

  static bool classof(const Expr *E) { return E->getKind() == Expr::Constant; }
  static bool classof(const ConstantExpr *) { return true; }

  /* Utility Functions */

  /// isZero - Is this a constant zero.
  bool isZero() const { assert(type == Integer); return getAPValue().isMinValue(); }

  /// isOne - Is this a constant one.
  bool isOne() const { assert(type == Integer); return getLimitedValue() == 1; }

  /// isTrue - Is this the true expression.
  bool isTrue() const {
    assert(type == Integer);
    return (getWidth() == Expr::Bool && value.getBoolValue() == true);
  }

  /// isFalse - Is this the false expression.
  bool isFalse() const {
    assert(type == Integer);
    return (getWidth() == Expr::Bool && value.getBoolValue() == false);
  }

  /// isAllOnes - Is this constant all ones.
  bool isAllOnes() const { assert(type == Integer); return getAPValue().isAllOnesValue(); }

  /* Constant Operations */

  ref<ConstantExpr> Concat(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Extract(unsigned offset, Width W);
  ref<ConstantExpr> ZExt(Width W);
  ref<ConstantExpr> SExt(Width W);
  ref<ConstantExpr> FToU(Width W, llvm::APFloat::roundingMode RM);
  ref<ConstantExpr> FToS(Width W, llvm::APFloat::roundingMode RM);
  ref<ConstantExpr> FpClassify();
  ref<ConstantExpr> FIsFinite();
  ref<ConstantExpr> FIsNan();
  ref<ConstantExpr> FIsInf();
  ref<ConstantExpr> Add(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sub(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Mul(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> UDiv(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> SDiv(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> URem(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> SRem(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> And(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Or(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Xor(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Shl(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> LShr(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> AShr(const ref<ConstantExpr> &RHS);

  // Comparisons return a constant expression of width 1.

  ref<ConstantExpr> Eq(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ne(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ult(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ule(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ugt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Uge(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Slt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sle(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sgt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sge(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOrd(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUno(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUeq(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOeq(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUgt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOgt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUge(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOge(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUlt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOlt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUle(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOle(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FUne(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> FOne(const ref<ConstantExpr> &RHS);

  ref<ConstantExpr> Neg();
  ref<ConstantExpr> Not();
};

class FloatExpr : public Expr {
public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return k >= Expr::Float && k <= Expr::LastKind;
  }
  static bool classof(const FloatExpr *) { return true; }
};

class FloatNonConstantExpr : public FloatExpr {
public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return k >= Expr::Float && k <= Expr::LastKind && k != Expr::FloatConstant;
  }
  static bool classof(const FloatNonConstantExpr *) { return true; }
};

class FloatCastExpr : public FloatNonConstantExpr {
public:
  ref<Expr> src;
  Width width;

public:
  FloatCastExpr(const ref<Expr> &e, Width w) : src(e), width(w) {}

  Width getWidth() const { return width; }

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return (i == 0) ? src : 0; }

  static bool needsResultType() { return true; }

  int compareContents(const Expr &b) const {
    const FloatCastExpr &eb = static_cast<const FloatCastExpr&>(b);
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  virtual unsigned computeHash();

  static bool classof(const Expr *E) {
    Expr::Kind k = E->getKind();
    return Expr::FloatCastKindFirst <= k && k <= Expr::FloatCastKindLast;
  }
  static bool classof(const FloatCastExpr *) { return true; }
};

class FloatCastRoundExpr : public FloatCastExpr {
public:
  llvm::APFloat::roundingMode round;

public:
  FloatCastRoundExpr(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm) : FloatCastExpr(e, w), round(rm) {}

  llvm::APFloat::roundingMode getRoundingMode() const { return round; }

  int compareContents(const Expr &b) const {
    const FloatCastRoundExpr &eb = static_cast<const FloatCastRoundExpr&>(b);
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  static bool classof(const Expr *E) {
    Expr::Kind k = E->getKind();
    return Expr::FloatCastRoundKindFirst <= k && k <= Expr::FloatCastRoundKindLast;
  }
  static bool classof(const FloatCastRoundExpr *) { return true; }
};

#define FLOAT_CAST_RM_EXPR_CLASS(_class_kind)                           \
class _class_kind ## Expr : public FloatCastRoundExpr {                                \
public:                                                                           \
  static const Kind kind = _class_kind;                                           \
  static const unsigned numKids = 1;                                              \
  static const llvm::APFloat::roundingMode round = llvm::APFloat::rmNearestTiesToEven;                          \
public:                                                                           \
    _class_kind ## Expr(ref<Expr> e, Width w, llvm::APFloat::roundingMode rm) : FloatCastRoundExpr(e,w,rm) {} \
    static ref<Expr> alloc(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm) {        \
      ref<Expr> r(new _class_kind ## Expr(e, w, rm));                             \
      r->computeHash();                                                           \
      return r;                                                                   \
    }                                                                             \
    static ref<Expr> create(const ref<Expr> &e, Width w, llvm::APFloat::roundingMode rm);        \
    Kind getKind() const { return _class_kind; }                                  \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                           \
      return create(kids[0], width, round);                                       \
    }                                                                             \
                                                                                  \
    Type getType() const { return Expr::FloatingPoint; }                                    \
                                                                                  \
    static bool classof(const Expr *E) {                                          \
      return E->getKind() == Expr::_class_kind;                                   \
    }                                                                             \
    static bool classof(const  _class_kind ## Expr *) {                           \
      return true;                                                                \
    }                                                                             \
};                                                                                \

FLOAT_CAST_RM_EXPR_CLASS(FExt)
FLOAT_CAST_RM_EXPR_CLASS(UToF)
FLOAT_CAST_RM_EXPR_CLASS(SToF)

class FloatUnaryExpr : public FloatNonConstantExpr {
public:
  ref<Expr> expr;

public:
  FloatUnaryExpr(const ref<Expr> &e) : expr(e) {}

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return (i == 0) ? expr : 0; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::FloatUnaryKindFirst <= k && k <= Expr::FloatUnaryKindLast;
  }
  static bool classof(const FloatUnaryExpr *) { return true; }
};

#define FLOAT_UNARY_EXPR_CLASS(_class_kind)              \
class _class_kind ## Expr : public UnaryExpr {           \
public:                                                  \
  static const Kind kind = _class_kind;                  \
  static const unsigned numKids = 1;                     \
public:                                                  \
    _class_kind ## Expr(ref<Expr> e) : UnaryExpr(e) {}   \
    static ref<Expr> alloc(const ref<Expr> &e) {         \
      ref<Expr> r(new _class_kind ## Expr(e));           \
      r->computeHash();                                  \
      return r;                                          \
    }                                                    \
    static ref<Expr> create(const ref<Expr> &e);         \
    Width getWidth() const { return expr->getWidth(); }  \
    Kind getKind() const { return _class_kind; }         \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {  \
      return create(kids[0]);                            \
    }                                                    \
                                                         \
    Type getType() const { return Expr::FloatingPoint; } \
                                                         \
    static bool classof(const Expr *E) {                 \
      return E->getKind() == Expr::_class_kind;          \
    }                                                    \
    static bool classof(const  _class_kind ## Expr *) {  \
      return true;                                       \
    }                                                    \
};

FLOAT_UNARY_EXPR_CLASS(FAbs)


class FloatUnaryRoundExpr : public FloatUnaryExpr {
public:
  llvm::APFloat::roundingMode round;

public:
  FloatUnaryRoundExpr(const ref<Expr> &e, llvm::APFloat::roundingMode rm) : FloatUnaryExpr(e), round(rm) {}

  llvm::APFloat::roundingMode getRoundingMode() const { return round; }

  Type getType() const { return Expr::FloatingPoint; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::FloatUnaryRoundKindFirst <= k && k <= Expr::FloatUnaryRoundKindLast;
  }
  static bool classof(const FloatUnaryRoundExpr *) { return true; }
};

#define FLOAT_UNARY_ROUND_EXPR_CLASS(_class_kind)                                                   \
class _class_kind ## Expr : public FloatUnaryRoundExpr {                                            \
public:                                                                                             \
  static const Kind kind = _class_kind;                                                             \
  static const unsigned numKids = 1;                                                                \
  static const llvm::APFloat::roundingMode round;                                                   \
public:                                                                                             \
    _class_kind ## Expr(ref<Expr> e, llvm::APFloat::roundingMode rm) : FloatUnaryRoundExpr(e,rm) {} \
    static ref<Expr> alloc(const ref<Expr> &e, llvm::APFloat::roundingMode rm) {                    \
      ref<Expr> r(new _class_kind ## Expr(e, rm));                                                  \
      r->computeHash();                                                                             \
      return r;                                                                                     \
    }                                                                                               \
    static ref<Expr> create(const ref<Expr> &e, llvm::APFloat::roundingMode rm);                    \
    Width getWidth() const { return expr->getWidth(); }                                             \
    Kind getKind() const { return _class_kind; }                                                    \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                                             \
      return create(kids[0], round);                                                                \
    }                                                                                               \
                                                                                                    \
    static bool classof(const Expr *E) {                                                            \
      return E->getKind() == Expr::_class_kind;                                                     \
    }                                                                                               \
    static bool classof(const  _class_kind ## Expr *) {                                             \
      return true;                                                                                  \
    }                                                                                               \
};

FLOAT_UNARY_ROUND_EXPR_CLASS(FSqrt)
FLOAT_UNARY_ROUND_EXPR_CLASS(FNearbyInt)

class FloatBinaryExpr : public FloatNonConstantExpr {
public:
  ref<Expr> left, right;

public:
  unsigned getNumKids() const { return 2; }
  ref<Expr> getKid(unsigned i) const {
    if (i == 0)
      return left;
    if (i == 1)
      return right;
    return 0;
  }

protected:
  FloatBinaryExpr(const ref<Expr> &l, const ref<Expr> &r) : left(l), right(r) {}

public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::FloatBinaryKindFirst <= k && k <= Expr::FloatBinaryKindLast;
  }
  static bool classof(const FloatBinaryExpr *) { return true; }
};

#define FLOAT_BINARY_EXPR_CLASS(_class_kind)                         \
class _class_kind ## Expr : public FloatBinaryExpr {                 \
public:                                                              \
  static const Kind kind = _class_kind;                              \
  static const unsigned numKids = 2;                                 \
public:                                                              \
    _class_kind ## Expr(const ref<Expr> &l,                          \
                        const ref<Expr> &r) : FloatBinaryExpr(l,r) {}\
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) { \
      ref<Expr> res(new _class_kind ## Expr (l, r));                 \
      res->computeHash();                                            \
      return res;                                                    \
    }                                                                \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r); \
    Width getWidth() const { return left->getWidth(); }              \
    Kind getKind() const { return _class_kind; }                     \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {              \
      return create(kids[0], kids[1]);                               \
    }                                                                \
                                                                     \
    Type getType() const { return Expr::Integer; }                   \
                                                                     \
    static bool classof(const Expr *E) {                             \
      return E->getKind() == Expr::_class_kind;                      \
    }                                                                \
    static bool classof(const  _class_kind ## Expr *) {              \
      return true;                                                   \
    }                                                                \
};                                                                   \

FLOAT_BINARY_EXPR_CLASS(FMin)
FLOAT_BINARY_EXPR_CLASS(FMax)

class FloatBinaryRoundExpr : public FloatBinaryExpr {
public:
  llvm::APFloat::roundingMode round;

protected:
  FloatBinaryRoundExpr(const ref<Expr> &l, const ref<Expr> &r, llvm::APFloat::roundingMode rm) : FloatBinaryExpr(l, r), round(rm) {}

public:
  llvm::APFloat::roundingMode getRoundingMode() const { return round; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::FloatBinaryRoundKindFirst <= k && k <= Expr::FloatBinaryRoundKindLast;
  }
  static bool classof(const FloatBinaryRoundExpr *) { return true; }
};


#define FLOAT_BINARY_RM_EXPR_CLASS(_class_kind)                                                      \
class _class_kind ## Expr : public FloatBinaryRoundExpr {                                            \
public:                                                                                              \
  static const Kind kind = _class_kind;                                                              \
  static const unsigned numKids = 2;                                                                 \
  static const llvm::APFloat::roundingMode round = llvm::APFloat::rmNearestTiesToEven;               \
public:                                                                                              \
    _class_kind ## Expr(const ref<Expr> &l,                                                          \
                        const ref<Expr> &r,                                                          \
                        llvm::APFloat::roundingMode rm) : FloatBinaryRoundExpr(l,r,rm) {}            \
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r, llvm::APFloat::roundingMode rm) { \
      ref<Expr> res(new _class_kind ## Expr (l, r, rm));                                             \
      res->computeHash();                                                                            \
      return res;                                                                                    \
    }                                                                                                \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r, llvm::APFloat::roundingMode rm); \
    Width getWidth() const { return left->getWidth(); }                                              \
    Kind getKind() const { return _class_kind; }                                                     \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {                                              \
      return create(kids[0], kids[1], round);                                                        \
    }                                                                                                \
                                                                                                     \
    Type getType() const { return Expr::FloatingPoint; }                                             \
                                                                                                     \
    static bool classof(const Expr *E) {                                                             \
      return E->getKind() == Expr::_class_kind;                                                      \
    }                                                                                                \
    static bool classof(const  _class_kind ## Expr *) {                                              \
      return true;                                                                                   \
    }                                                                                                \
};                                                                                                   \

FLOAT_BINARY_RM_EXPR_CLASS(FAdd)
FLOAT_BINARY_RM_EXPR_CLASS(FSub)
FLOAT_BINARY_RM_EXPR_CLASS(FMul)
FLOAT_BINARY_RM_EXPR_CLASS(FDiv)
FLOAT_BINARY_RM_EXPR_CLASS(FRem)

class FloatConstantExpr : public FloatExpr {
public:
  static const Kind kind = FloatConstant;
  static const unsigned numKids = 0;

private:
  llvm::APInt value;

  Type type;

  FloatConstantExpr(const llvm::APInt &v, Expr::Type t) : value(v), type(t) {}

public:
  ~FloatConstantExpr() {}

  Width getWidth() const { return value.getBitWidth(); }
  Type getType() const { return type; }
  Kind getKind() const { return FloatConstant; }

  unsigned getNumKids() const { return 0; }
  ref<Expr> getKid(unsigned i) const { return 0; }

  /// getAPValue - Return the arbitrary precision value directly.
  ///
  /// Clients should generally not use the APInt value directly and instead use
  /// native FloatConstantExpr APIs.
  const llvm::APInt &getAPValue() const { return value; }

  /// getZExtValue - Returns the constant value zero extended to the
  /// return type of this method.
  ///
  ///\param bits - optional parameter that can be used to check that the
  /// number of bits used by this constant is <= to the parameter
  /// value. This is useful for checking that type casts won't truncate
  /// useful bits.
  ///
  /// Example: unit8_t byte= (unit8_t) constant->getZExtValue(8);
  uint64_t getZExtValue(unsigned bits = 64) const {
    assert(getWidth() <= bits && "Value may be out of range!");
    return value.getZExtValue();
  }

  /// getLimitedValue - If this value is smaller than the specified limit,
  /// return it, otherwise return the limit value.
  uint64_t getLimitedValue(uint64_t Limit = ~0ULL) const {
    return value.getLimitedValue(Limit);
  }

  /// toString - Return the constant value as a string
  /// \param Res specifies the string for the result to be placed in
  /// \param radix specifies the base (e.g. 2,10,16). The default is base 10
  void toString(std::string &Res, unsigned radix = 10) const;

  int compareContents(const Expr &b) const {
    const FloatConstantExpr &cb = static_cast<const FloatConstantExpr &>(b);
    if (getWidth() != cb.getWidth())
      return getWidth() < cb.getWidth() ? -1 : 1;
    if (value == cb.value)
      return 0;
    return value.ult(cb.value) ? -1 : 1;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const {
    assert(0 && "rebuild() on FloatConstantExpr");
    return const_cast<FloatConstantExpr *>(this);
  }

  virtual unsigned computeHash();

  static ref<Expr> fromMemory(void *address, Width w);
  void toMemory(void *address);

  static ref<FloatConstantExpr> alloc(const llvm::APInt &v, Expr::Type t) {
    ref<FloatConstantExpr> r(new FloatConstantExpr(v, t));
    r->computeHash();
    return r;
  }

  static ref<FloatConstantExpr> alloc(const llvm::APInt &v) {
    return alloc(v, Integer);
  }

  static ref<FloatConstantExpr> alloc(const llvm::APFloat &f) {
    return alloc(f.bitcastToAPInt(), FloatingPoint);
  }

  static ref<FloatConstantExpr> alloc(uint64_t v, Width w) {
    return alloc(llvm::APInt(w, v));
  }

  static ref<FloatConstantExpr> create(uint64_t v, Width w) {
    assert(v == bits64::truncateToNBits(v, w) && "invalid constant");
    return alloc(v, w);
  }

  static bool classof(const Expr *E) { return E->getKind() == Expr::FloatConstant; }
  static bool classof(const FloatConstantExpr *) { return true; }

  /* Constant Operations */

  ref<FloatConstantExpr> FExt(Width W, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> UToF(Width W, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> SToF(Width W, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FAbs();
  ref<FloatConstantExpr> FSqrt(llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FNearbyInt(llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FAdd(const ref<FloatConstantExpr> &RHS, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FSub(const ref<FloatConstantExpr> &RHS, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FMul(const ref<FloatConstantExpr> &RHS, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FDiv(const ref<FloatConstantExpr> &RHS, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FRem(const ref<FloatConstantExpr> &RHS, llvm::APFloat::roundingMode RM);
  ref<FloatConstantExpr> FMin(const ref<FloatConstantExpr> &RHS);
  ref<FloatConstantExpr> FMax(const ref<FloatConstantExpr> &RHS);

};


// Implementations

inline bool Expr::isZero() const {
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isZero();
  return false;
}
  
inline bool Expr::isTrue() const {
  assert(getWidth() == Expr::Bool && "Invalid isTrue() call!");
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isTrue();
  return false;
}
  
inline bool Expr::isFalse() const {
  assert(getWidth() == Expr::Bool && "Invalid isFalse() call!");
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isFalse();
  return false;
}

} // End klee namespace

#endif
