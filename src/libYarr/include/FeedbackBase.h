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
typedef std::map<unsigned, FeedbackClipboard> FeedbackClipboardMap;

class GlobalFeedbackBase {
    public:
        virtual ~GlobalFeedbackBase() = default;
        virtual void feedback(unsigned channel, double sign, bool last) = 0;
        virtual void feedbackBinary(unsigned channel, double sign, bool last) = 0; // TODO Algorithm should be selected in scan
        virtual void feedbackStep(unsigned channel, double sign, bool last) {}
};

class GlobalFeedbackReceiver : public GlobalFeedbackBase {
    public:
        GlobalFeedbackReceiver() : clip(nullptr) {}
        void connectClipboard(FeedbackClipboardMap *fe);

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback(unsigned ch);

        /// Check all fbDoneMap entries
        bool isFeedbackDone() const;

        /// Commonly used to store 'last' flag from feedback invocation
        std::map<unsigned, bool> fbDoneMap;

    private:
        FeedbackClipboardMap *clip;
};

/// Generator of feedback
class GlobalFeedbackSender : public GlobalFeedbackBase {
    public:
        GlobalFeedbackSender(FeedbackClipboard *fb);

        void feedback(unsigned channel, double sign, bool last) override;
        void feedbackBinary(unsigned channel, double sign, bool last) override;
        void feedbackStep(unsigned channel, double sign, bool last) override;

    private:
        FeedbackClipboard *clip;
};

class PixelFeedbackBase {
    public:
        virtual ~PixelFeedbackBase() = default;
        virtual void feedback(unsigned channel, std::unique_ptr<Histo2d> h) {};
        virtual void feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h) {};
};

class PixelFeedbackReceiver : public PixelFeedbackBase {
    public:
        PixelFeedbackReceiver() : clip(nullptr) {}

        void connectClipboard(FeedbackClipboardMap *fe);

    protected:
        /// Wait for feedback to be received and apply it
        void waitForFeedback(unsigned ch);

        /// Check all fbDoneMap entries
        bool isFeedbackDone() const;

        /// Commonly used to store 'last' flag from feedback invocation
        std::map<unsigned, bool> fbDoneMap;

    private:
        FeedbackClipboardMap *clip;
};

class PixelFeedbackSender : public PixelFeedbackBase {
    public:
        PixelFeedbackSender(FeedbackClipboard *fb);

        void feedback(unsigned channel, std::unique_ptr<Histo2d> h) override;
        void feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h) override;

    private:
        FeedbackClipboard *clip;
};

#endif
