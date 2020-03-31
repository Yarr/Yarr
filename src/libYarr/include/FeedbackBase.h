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

struct FeedbackParams {
    bool step;
    bool binary;

    PixelFeedbackParams &pixel() {
        return std::get<PixelFeedbackParams>(data);
    }

    GlobalFeedbackParams &global() {
        return std::get<GlobalFeedbackParams>(data);
    }

    std::variant<GlobalFeedbackParams, PixelFeedbackParams> data;

    FeedbackParams(bool step, bool binary, std::variant<GlobalFeedbackParams, PixelFeedbackParams> &&data)
      : step(step), binary(binary), data(std::move(data)) {}
};

// Make it shared so controller can still observe
typedef ClipBoard<FeedbackParams> FeedbackClipboard;

class GlobalFeedbackBase {
    public:
        virtual void feedback(unsigned channel, double sign, bool last) = 0;
        virtual void feedbackBinary(unsigned channel, double sign, bool last) = 0; // TODO Algorithm should be selected in scan
        virtual void feedbackStep(unsigned channel, double sign, bool last) {}
};

class GlobalFeedbackReceiver : public GlobalFeedbackBase {
    public:
        GlobalFeedbackReceiver() : clip(nullptr) {}
        void connectClipboard(FeedbackClipboard *fe) { clip = fe; }

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback(unsigned ch);

    private:
        FeedbackClipboard *clip;
};

/// Generator of feedback
class GlobalFeedbackSender : public GlobalFeedbackBase {
    public:
        GlobalFeedbackSender() : clip(nullptr) {}

        void connectClipboard(FeedbackClipboard *fe) { clip = fe; }

        void feedback(unsigned channel, double sign, bool last);
        void feedbackBinary(unsigned channel, double sign, bool last);
        void feedbackStep(unsigned channel, double sign, bool last);

    private:
        FeedbackClipboard *clip;
};

class PixelFeedbackBase {
    public:
        virtual void feedback(unsigned channel, std::unique_ptr<Histo2d> h) {};
        virtual void feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h) {};
};

class PixelFeedbackReceiver : public PixelFeedbackBase {
    public:
        PixelFeedbackReceiver() : clip(nullptr) {}

        void connectClipboard(FeedbackClipboard *fe) { clip = fe; }

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback(unsigned ch);

    private:
        FeedbackClipboard *clip;
};

class PixelFeedbackSender : public PixelFeedbackBase {
    public:
        PixelFeedbackSender() : clip(nullptr) {}

        void connectClipboard(FeedbackClipboard *fe) { clip = fe; }

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h);
        void feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h);

    private:
        FeedbackClipboard *clip;
};

#endif
