/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef ENGINETBASE_H
#define ENGINETBASE_H

/**
 * Base class providing helpers for LoopEngine.
 */
template <typename LT>
class EngineTBase {
	public: 
		/** Record template argument */
		typedef LT loop_list_type;
		/** Extract type of list elements */
		typedef typename LT::value_type element_value_type;
		/** Connect loop actions together and execute them */
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

		/** Add LoopAction to list */
		static void addItem( loop_list_type& list, element_value_type el ) {
			list.push_back(el);
		}
};

#endif
