#pragma once
#include <stdint.h>

// Chainlist with only one pointer to the next member.
// ADVANTAGES:    -Smaller memory imprint;
// DISADVANTAGES: -no pointer to the previous member (obviously)
//                -del is SLOW: it has to pass thru the whole list to find the previous member for the links to work

// derive your 'data class' from this 'base class'
class onewayData {
public:
  onewayData *next;
};


// list 'handler' - create a variable from this class to handle the list
class onewayList {
public:
  onewayData *first;
  inline onewayData *last() { onewayData *p= first; while(p->next) p= p->next; return p; }
  uint32_t nrNodes;               // number of nodes in the list
  
  // fast funcs

  inline void add(onewayData *);         // [FAST] alloc mem, then pass pointer to add(). _better/faster function_
  inline void addFirst(onewayData *);    // [FAST] adds the node to the front of the list not the back (makes it first)
  
  // slow funcs

  inline void del(onewayData *);         /// [SLOW] searches involved - SLOW
  inline void deli(uint32_t);            /// [SLOW] searches involved - SLOW - good to have tho
  inline onewayData *get(uint32_t);      /// [SLOW] searches involved - SLOW
  inline uint32_t search(onewayData *);  /// [SLOW] returns -1 if failed to find;

  inline onewayList(): first(nullptr), nrNodes(0) {}
  inline ~onewayList() { delData(); }
  inline void delData() { while(first) del(first); }
};


// must alloc from code, then call this func.
///------------------------------------------

void onewayList::add(onewayData *p) {		
  if(first) last()->next= p;  // SLOW PART
  else first= p;              /// first link
  p->next= nullptr;           /// do not depend on constructor, it won't be called in the derived class; nice source of errors
  nrNodes++;
}


void onewayList::addFirst(onewayData *p) {
  p->next= first;
  first= p;
  nrNodes++;
}

// slower - passthru all list to find the prev link 
void onewayList::del(onewayData *p) {
  if(p== first)
    first= p->next;
  else
    for(onewayData *prev= first; ; prev= prev->next)      // <<< SLOW PART >>>
      if(prev->next== p) { prev->next= p->next; break; }  // found it

  /// delete
  delete p;
  nrNodes--;
}

// slow - goes thru the list, with many instructions per cicle
void onewayList::deli(uint32_t nr) {
  onewayData *p= first;
  if(nr> 0) {
    onewayData *prev= first;
    for(; nr> 1; --nr, prev= prev->next)            // <<< SLOW PART >>>
    p= prev->next;
    prev->next= p->next;
  } else
    first= first->next;

  delete p;
  nrNodes--;
}

// get must be used rarely
onewayData *onewayList::get(uint32_t nr) {
  onewayData *p= first;
  while(nr--)                                       // <<< SLOW PART >>>
    p= p->next;

  return p;
}

// same as get, use RARELY
uint32_t onewayList::search(onewayData *e) {
  onewayData *p= first;
  for(uint32_t a= 0; a< nrNodes; a++, p= p->next)   // <<< SLOW PART >>>
    if(p== e) return a;

  return ~0;
}






