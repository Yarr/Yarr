/*
 * Authors: K. Potamianos <karolos.potamianos@cern.ch>,
 *          T. Heim <timon.heim@cern.ch>
 * Date: 2013-Oct-22
 */

#ifndef LOOPACTIONBASE_H
#define LOOPACTIONBASE_H

#include <memory>
using std::shared_ptr;

class LoopActionBase {
    public:
        LoopActionBase();

        void setNext(shared_ptr<LoopActionBase>& ptr);
        void execute();
        
    protected:
        virtual void init() {}
        virtual void end() {}
        virtual void execPart1() {}
        virtual void execPart2() {}
        virtual bool done();

        bool m_done;

    private:
        void execStep();
        void run();

        shared_ptr<LoopActionBase> m_inner;
};
#endif
