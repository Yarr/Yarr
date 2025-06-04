#ifndef YARR_FRONT_END_CLIPBOARDS_H
#define YARR_FRONT_END_CLIPBOARDS_H

#include "ClipBoard.h"

#include "RawData.h"
#include "EventDataBase.h"
#include "HistogramBase.h"
#include "ResultBase.h"

/// Clipboards to buffer data between processors
struct FrontEndClipBoards {
    ClipBoard<RawDataContainer> clipRawData;
    ClipBoard<EventDataBase> clipData;
    ClipBoard<HistogramBase> clipHisto;
    ClipBoard<FeedbackProcessingInfo> clipProcFeedback;
    std::vector<std::unique_ptr<ClipBoard<HistogramBase>> > clipResult;
};

#endif
