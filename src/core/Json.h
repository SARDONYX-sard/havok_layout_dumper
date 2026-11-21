#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Minimal ordered pretty JSON writer (2-space indent), matching the formatting
// of serde-hkx `assets/classes/*.json`.

namespace Json {
    class Writer {
    public:
        Writer& begin_object() { return open('{'); }
        Writer& end_object() { return close('}'); }
        Writer& begin_array() { return open('['); }
        Writer& end_array() { return close(']'); }

        Writer& key(std::string_view a_key) {
            separate();
            write_string(a_key);
            _out += ": ";
            _afterKey = true;
            return *this;
        }

        Writer& value(std::string_view a_value) {
            separate();
            write_string(a_value);
            return *this;
        }

        Writer& value(const std::string& a_value) { return value(std::string_view{a_value}); }
        Writer& value(const char* a_value) { return value(std::string_view{a_value}); }

        Writer& value(bool a_value) {
            separate();
            _out += a_value ? "true" : "false";
            return *this;
        }

        Writer& value(std::int64_t a_value) {
            separate();
            _out += std::to_string(a_value);
            return *this;
        }

        [[nodiscard]] std::string finish() {
            _out += '\n';
            return std::move(_out);
        }

    private:
        struct Scope {
            bool empty{true};
        };

        Writer& open(char a_bracket) {
            separate();
            _out += a_bracket;
            _scopes.push_back({});
            return *this;
        }

        Writer& close(char a_bracket) {
            const bool empty = _scopes.back().empty;
            _scopes.pop_back();
            if (!empty) {
                newline();
            }
            _out += a_bracket;
            return *this;
        }

        /// Emits `,` + newline + indent before a new element (unless directly after a key).
        void separate() {
            if (_afterKey) {
                _afterKey = false;
                return;
            }
            if (_scopes.empty()) {
                return;
            }
            auto& scope = _scopes.back();
            if (!scope.empty) {
                _out += ',';
            }
            scope.empty = false;
            newline();
        }

        void newline() {
            _out += '\n';
            _out.append(_scopes.size() * 2, ' ');
        }

        void write_string(std::string_view a_str) {
            _out += '"';
            for (const char c : a_str) {
                switch (c) {
                case '"':
                    _out += "\\\"";
                    break;
                case '\\':
                    _out += "\\\\";
                    break;
                case '\n':
                    _out += "\\n";
                    break;
                case '\t':
                    _out += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        _out += std::format("\\u{:04x}", static_cast<unsigned char>(c));
                    } else {
                        _out += c;
                    }
                    break;
                }
            }
            _out += '"';
        }

        std::string _out;
        std::vector<Scope> _scopes;
        bool _afterKey{false};
    };
}
