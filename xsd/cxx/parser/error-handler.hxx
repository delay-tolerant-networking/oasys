// file      : xsd/cxx/parser/error-handler.hxx
// author    : Boris Kolpackov <boris@codesynthesis.com>
// copyright : Copyright (c) 2005-2006 Code Synthesis Tools CC
// license   : GNU GPL v2 + exceptions; see accompanying LICENSE file

#ifndef XSD_CXX_PARSER_ERROR_HANDLER_HXX
#define XSD_CXX_PARSER_ERROR_HANDLER_HXX

#include <xsd/cxx/xml/error-handler.hxx>

#include <xsd/cxx/parser/exceptions.hxx>

namespace xsd
{
  namespace cxx
  {
    namespace parser
    {
      template <typename C>
      class error_handler: public xml::error_handler<C>
      {
        typedef typename xml::error_handler<C>::severity severity;

      public:
        virtual bool
        handle (const std::basic_string<C>& id,
                unsigned long line,
                unsigned long column,
                severity s,
                const std::basic_string<C>& message)
        {
          if (s == severity::error || s == severity::fatal)
            errors_.push_back (error<C> (id, line, column, message));

          return true;
        }

        void
        throw_if_failed () const
        {
          if (!errors_.empty ())
            throw parsing<C> (errors_);
        }

      private:
        errors<C> errors_;
      };
    }
  }
}

#endif  // XSD_CXX_PARSER_ERROR_HANDLER_HXX