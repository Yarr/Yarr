#ifndef QDEBUGSTREAM_H
#define QDEBUGSTREAM_H

#include <iostream>
#include <streambuf>
#include <string>

#include <QTextEdit>
#include <QTime>
#include <QColor>

class QDebugStream : public std::basic_streambuf<char>
{
    public:
        QDebugStream(std::ostream &stream, QTextEdit* text_edit, QColor arg_color=QColor("black")) : m_stream(stream)
    {
        log_window = text_edit;
        color = arg_color;
        m_old_buf = stream.rdbuf();
        stream.rdbuf(this);
    }
        ~QDebugStream()
        {
            // output anything that is left
            if (!m_string.empty()) {
                QTime local(QTime::currentTime());
                log_window->setTextColor(color);
                log_window->append("[" + local.toString("hh:mm:ss.zzz") + "] " + m_string.c_str());
            }

            m_stream.rdbuf(m_old_buf);
        }

    protected:
        virtual int_type overflow(int_type v)
        {
            if (v == '\n')
            {
                QTime local(QTime::currentTime());
                log_window->setTextColor(color);
                log_window->append("[" + local.toString("hh:mm:ss.zzz") + "] " + m_string.c_str());
                m_string.erase(m_string.begin(), m_string.end());
            }
            else
                m_string += v;

            return v;
        }

        virtual std::streamsize xsputn(const char *p, std::streamsize n)
        {
            m_string.append(p, p + n);

            int pos = 0;
            while (pos != std::string::npos)
            {
                pos = m_string.find('\n');
                if (pos != std::string::npos)
                {
                    QTime local(QTime::currentTime());
                    log_window->setTextColor(color);
                    std::string tmp(m_string.begin(), m_string.begin() + pos);
                    log_window->append("[" + local.toString("hh:mm:ss.zzz") + "] " + tmp.c_str());
                    m_string.erase(m_string.begin(), m_string.begin() + pos + 1);
                }
            }

            return n;
        }

    private:
        std::ostream &m_stream;
        std::streambuf *m_old_buf;
        std::string m_string;


        QTextEdit* log_window;
        QColor color;
};

#endif
