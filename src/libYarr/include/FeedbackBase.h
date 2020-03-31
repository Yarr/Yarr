/*
 * Authors: T. Heim <timon.heim@cern.ch>,
 * Date: 2014-Nov-24
 */

#ifndef FEEDBACKBASE_H
#define FEEDBACKBASE_H

#include <variant>

#include "ClipBoard.h"
#include "Histo2d.h"

struct GlobalFeedbackParams {
    double sign;
    bool last;
};

struct PixelFeedbackParams {
    std::unique_ptr<Histo2d> histo;
};

typedef std::variant<GlobalFeedbackParams, PixelFeedbackParams> FeedbackParams;

// Make it shared so controller can still observe
typedef std::shared_ptr<Clipboard<FeedbackParams>> FeedbackClipboard;

class GlobalFeedbackBase {
    public:
        virtual void feedback(unsigned channel, double sign, bool last) = 0;
        virtual void feedbackBinary(unsigned channel, double sign, bool last) = 0; // TODO Algorithm should be selected in scan
        virtual void feedbackStep(unsigned channel, double sign, bool last) {}
};

class GlobalFeedbackReceiver : public GlobalFeedbackBase {
    public:
        void connectClipboard(FeedbackClipboard fe) { clip = fe; }

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback();

    private:
        FeedbackClipboard clip;
};

/// Generator of feedback
class GlobalFeedbackSender : public GlobalFeedbackBase {
    public:
        void connectClipboard(FeedbackClipboard fe) { clip = fe; }

        void feedback(unsigned channel, double sign, bool last);
        void feedbackBinary(unsigned channel, double sign, bool last);
        void feedbackStep(unsigned channel, double sign, bool last);
};

class PixelFeedbackBase {
    public:
        virtual void feedback(unsigned channel, std::unique_ptr<Histo2d> h) {};
        virtual void feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h) {};

        void connectClipboard(FeedbackClipboard fe) { clip = fe; }

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback();

    private:
        FeedbackClipboard clip;
};

#endif
