#include "FeedbackBase.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Feedback");
}

void GlobalFeedbackReceiver::connectClipboard(FeedbackClipboardMap *fe) {
    logger->trace("Connect clipboard map for GlobalFeedbackReceiver");
    clip = fe;
}

void GlobalFeedbackReceiver::waitForFeedback(unsigned channel) {
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->error("Request waiting for global feedback when no pipe connected ");
        throw std::runtime_error("Missing feedback connection");
    }

    auto ch_clip = clip->find(channel);

    if(ch_clip == clip->end()) {
        logger->error("Request waiting for global feedback when no pipe connected for channel {}", channel);
        throw std::runtime_error("Missing feedback channel connection");
    }

    auto &input = ch_clip->second;

    input.waitNotEmptyOrDone();
    auto fbData = input.popData();

    auto data = fbData->global();

    if(fbData->binary) {
        feedbackBinary(channel, data.sign, data.last);
    } else if(fbData->step) {
        feedbackStep(channel, data.sign, data.last);
    } else {
        feedback(channel, data.sign, data.last);
    }
}

void PixelFeedbackReceiver::connectClipboard(FeedbackClipboardMap *fe) {
    logger->trace("Connect clipboard map for PixelFeedbackReceiver");
    clip = fe;
}

void PixelFeedbackReceiver::waitForFeedback(unsigned channel) {
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->error("Request waiting for pixel feedback when no pipe connected ");
        throw std::runtime_error("Missing feedback connection");
    }

    auto ch_clip = clip->find(channel);

    if(ch_clip == clip->end()) {
        logger->error("Request waiting for pixel feedback when no pipe connected for channel {}", channel);
        throw std::runtime_error("Missing feedback channel connection");
    }

    auto &input = ch_clip->second;
    input.waitNotEmptyOrDone();
    auto fbData = input.popData();

    auto &data = fbData->pixel();
    if(fbData->step) {
        feedbackStep(channel, std::move(data.histo));
    } else {
        feedback(channel, std::move(data.histo));
    }
}

void GlobalFeedbackSender::connectClipboard(FeedbackClipboard *fe) {
  logger->trace("Connect clipboard for GlobalFeedbackSender");
    clip = fe;
}

void GlobalFeedbackSender::feedback(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no action configured
        logger->error("Sending feedback (global) with no pipe connected ");
        throw std::runtime_error("Missing feedback connection (feedback)");
    }

    // Where did the channel go! (there's an instance of this for each FE/AnalysisAlgorithm 

    logger->trace("Global feedback: {} {}", sign, last);

    GlobalFeedbackParams params{sign, last};
    std::variant<GlobalFeedbackParams, PixelFeedbackParams> v(params);

    auto fbParams = std::unique_ptr<FeedbackParams>
      (new FeedbackParams(false, false, std::move(v)));

    clip->pushData(std::move(fbParams));
}

void GlobalFeedbackSender::feedbackBinary(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no action configured
        logger->error("Sending feedbackBinary (global) with no pipe connected ");
        throw std::runtime_error("Missing feedback connection (feedbackBinary)");
    }

    logger->trace("Global feedbackBinary: {} {}", sign, last);
    GlobalFeedbackParams params{sign, last};

    auto fbParams = std::make_unique<FeedbackParams>(false, true, params);

    clip->pushData(std::move(fbParams));
}

void GlobalFeedbackSender::feedbackStep(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no action configured
        logger->error("Sending feedbackStep (global) with no pipe connected ");
        throw std::runtime_error("Missing feedback connection (feedbackStep)");
    }

    logger->trace("Global feedbackStep: {} {}", sign, last);
    GlobalFeedbackParams params{sign, last};

    auto fbParams = std::make_unique<FeedbackParams>(true, false, params);

    clip->pushData(std::move(fbParams));
}

void PixelFeedbackSender::connectClipboard(FeedbackClipboard *fe) {
    logger->trace("Connect clipboard for PixelFeedbackSender");
    clip = fe;
}

void PixelFeedbackSender::feedback(unsigned channel, std::unique_ptr<Histo2d> h)
{
    if(!clip) {
        // This is equivalent to no action configured
        logger->error("Sending feedback (pixel) with no pipe connected ");
        throw std::runtime_error("Missing feedback connection (feedback)");
    }

    logger->trace("Pixel feedback on channel {}", channel);
    PixelFeedbackParams params{std::move(h)};

    auto fbParams = std::make_unique<FeedbackParams>(false, false, std::move(params));

    clip->pushData(std::move(fbParams));
}

void PixelFeedbackSender::feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h)
{
    if(!clip) {
        // This is equivalent to no action configured
        logger->error("Sending feedbackStep (pixel) with no pipe connected ");
        throw std::runtime_error("Missing feedback connection (feedbackStep)");
    }

    logger->trace("Pixel feedbackStep on channel {}", channel);
    PixelFeedbackParams params{std::move(h)};

    auto fbParams = std::make_unique<FeedbackParams>(true, false, std::move(params));

    clip->pushData(std::move(fbParams));
}
