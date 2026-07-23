#ifndef YARR_PROGRESSAWARESINK_H
#define YARR_PROGRESSAWARESINK_H

#include <memory>
#include <mutex>

#include "spdlog/sinks/base_sink.h"

#include "ProgressBar.h"

/**
 * Wraps another spdlog sink so that every log write is bracketed by
 * erasing/redrawing the ProgressBar, keeping the progress block from
 * being corrupted by interleaved log output.
 */
class ProgressAwareSink : public spdlog::sinks::base_sink<std::mutex> {
    public:
        explicit ProgressAwareSink(std::shared_ptr<spdlog::sinks::sink> target)
            : m_target(std::move(target)) {}

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override {
            ProgressBar::instance().aroundExternalWrite([&]() {
                m_target->log(msg);
            });
        }

        void flush_() override {
            m_target->flush();
        }

        void set_pattern_(const std::string &pattern) override {
            m_target->set_pattern(pattern);
        }

        void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override {
            m_target->set_formatter(std::move(sink_formatter));
        }

    private:
        std::shared_ptr<spdlog::sinks::sink> m_target;
};

#endif
