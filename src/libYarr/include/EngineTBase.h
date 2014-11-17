/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef ENGINETBASE_H
#define ENGINETBASE_H

#include <memory>
using std::shared_ptr;

#include <vector>

template <typename LT>
class EngineTBase {
	public: 
		typedef LT loop_list_type;
		typedef typename LT::value_type element_value_type;
		static void execute( LT &task_list ) {
			typename loop_list_type::iterator it = task_list.begin();
			while(task_list.end() != it) {
				typename loop_list_type::iterator n_it = it+1;
				if(task_list.end() != n_it)
					(*it)->setNext( *(n_it) );
				++it;
			}
			it = task_list.begin();
			if(task_list.end() != it) 
				(*it)->execute();
		}
		static void addItem( loop_list_type& list, element_value_type el ) {
			list.push_back(el);
		}
};

#endif
