/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
 *
 * @author Troels Blum <troels@blum.dk>
 * @section LICENSE
 * This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *         but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * Data handler that simply throws data away - for performance testing purposes
 *
 */

#ifndef JADAQ_DATAWRITERNULL_HPP
#define JADAQ_DATAWRITERNULL_HPP

#include "DataFormat.hpp"
#include "container.hpp"
#include <cstdint>

class DataWriter {
public:
  DataWriter() = default;
  template <typename DW> DataWriter &operator=(DW *dataWriter) {
    instance.reset(new Model<DW>(dataWriter));
    return *this;
  }

  void addDigitizer(uint32_t digitizerID) {
    instance->addDigitizer(digitizerID);
  }

  /// \todo kill?
  void split(const std::string &id) { instance->split(id); }

  template <typename E>
  void operator()(const jadaq::buffer<E> *buffer, uint32_t digitizerID,
                  uint64_t globalTimeStamp) {
    instance->operator()(buffer, digitizerID, globalTimeStamp);
  }

private:
  struct Concept {
    virtual ~Concept() = default;
    virtual void addDigitizer(uint32_t digitizerID) = 0;
    virtual void split(const std::string &id) = 0;
    virtual void operator()(const jadaq::buffer<Data::ListElement422> *buffer,
                            uint32_t digitizerID, uint64_t globalTimeStamp) = 0;
    virtual void operator()(const jadaq::buffer<Data::ListElement8222> *buffer,
                            uint32_t digitizerID, uint64_t globalTimeStamp) = 0;
    virtual void
    operator()(const jadaq::buffer<Data::WaveformElement<Data::ListElement422>>
                   *buffer,
               uint32_t digitizerID, uint64_t globalTimeStamp) = 0;
    virtual void
    operator()(const jadaq::buffer<Data::WaveformElement<Data::ListElement8222>>
                   *buffer,
               uint32_t digitizerID, uint64_t globalTimeStamp) = 0;
  };
  template <typename DW> struct Model : Concept {
    explicit Model(DW *value) : val(value) {}
    ~Model() { delete val; }
    void addDigitizer(uint32_t digitizerID) override {
      val->addDigitizer(digitizerID);
    }
    void split(const std::string &id) override { return val->split(id); }
    void operator()(const jadaq::buffer<Data::ListElement422> *buffer,
                    uint32_t digitizerID, uint64_t globalTimeStamp) final {
      val->operator()(buffer, digitizerID, globalTimeStamp);
    }
    void operator()(const jadaq::buffer<Data::ListElement8222> *buffer,
                    uint32_t digitizerID, uint64_t globalTimeStamp) final {
      val->operator()(buffer, digitizerID, globalTimeStamp);
    }
    void
    operator()(const jadaq::buffer<Data::WaveformElement<Data::ListElement422>>
                   *buffer,
               uint32_t digitizerID, uint64_t globalTimeStamp) final {
      val->operator()(buffer, digitizerID, globalTimeStamp);
    }
    void
    operator()(const jadaq::buffer<Data::WaveformElement<Data::ListElement8222>>
                   *buffer,
               uint32_t digitizerID, uint64_t globalTimeStamp) final {
      val->operator()(buffer, digitizerID, globalTimeStamp);
    }
    DW *val;
  };

  std::unique_ptr<Concept> instance;
};

class DataWriterNull {
public:
  DataWriterNull() = default;
  void addDigitizer(uint32_t) {}
  void split(const std::string &) {}
  template <typename E>
  void operator()(const jadaq::buffer<E> *, uint32_t, uint64_t) {}
};

#endif // JADAQ_DATAWRITERNULL_HPP
