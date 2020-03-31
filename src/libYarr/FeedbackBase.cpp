#include "FeedbackBase.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("Feedback");
}

void GlobalFeedbackReceiver::waitForFeedback(unsigned channel) {
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Request waiting for feedback when no pipe connected ");
    }

    clip->waitNotEmptyOrDone();
    auto fbData = clip->popData();

    auto data = fbData->global();

    if(fbData->binary) {
        feedbackBinary(channel, data.sign, data.last);
    } else if(fbData->step) {
        feedbackStep(channel, data.sign, data.last);
    } else {
        feedback(channel, data.sign, data.last);
    }
}

void PixelFeedbackReceiver::waitForFeedback(unsigned channel) {
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Request waiting for feedback when no pipe connected ");
    }

    clip->waitNotEmptyOrDone();
    auto fbData = clip->popData();

    auto &data = fbData->pixel();
    if(fbData->step) {
        feedbackStep(channel, std::move(data.histo));
    } else {
        feedback(channel, std::move(data.histo));
    }
}

void GlobalFeedbackSender::feedback(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Sending feedback with no pipe connected ");
    }

    GlobalFeedbackParams params{sign, last};
    std::variant<GlobalFeedbackParams, PixelFeedbackParams> v(params);

    auto fbParams = std::unique_ptr<FeedbackParams>
      (new FeedbackParams(false, false, std::move(v)));

    clip->pushData(std::move(fbParams));
}

void GlobalFeedbackSender::feedbackBinary(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Sending feedback with no pipe connected ");
    }

    GlobalFeedbackParams params{sign, last};

    auto fbParams = std::make_unique<FeedbackParams>(false, true, params);

    clip->pushData(std::move(fbParams));
}

void GlobalFeedbackSender::feedbackStep(unsigned channel, double sign, bool last)
{
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Sending feedback with no pipe connected ");
    }

    GlobalFeedbackParams params{sign, last};

    auto fbParams = std::make_unique<FeedbackParams>(true, false, params);

    clip->pushData(std::move(fbParams));
}

void PixelFeedbackSender::feedback(unsigned channel, std::unique_ptr<Histo2d> h)
{
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Sending feedback with no pipe connected ");
    }

    PixelFeedbackParams params{std::move(h)};

    auto fbParams = std::make_unique<FeedbackParams>(false, false, std::move(params));

    clip->pushData(std::move(fbParams));
}

void PixelFeedbackSender::feedbackStep(unsigned channel, std::unique_ptr<Histo2d> h)
{
    if(!clip) {
        // This is equivalent to no analysis configured
        logger->warn("Sending feedback with no pipe connected ");
    }

    PixelFeedbackParams params{std::move(h)};

    auto fbParams = std::make_unique<FeedbackParams>(true, false, std::move(params));

    clip->pushData(std::move(fbParams));
}
