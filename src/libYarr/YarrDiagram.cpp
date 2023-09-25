#include "YarrDiagram.h"

#include <typeinfo>
#include <iomanip>

void YarrDiagram::makeDiagram(
  Bookkeeper& keeper,
  const std::map<unsigned, std::unique_ptr<FeDataProcessor>>& data_procs,
  const std::map<unsigned, std::unique_ptr<HistoDataProcessor>>& hist_procs,
  const std::map<unsigned, std::vector<std::unique_ptr<DataProcessor>>>& ana_procs
) {
  // Loop over front ends
  for (unsigned id = 0; id < keeper.getNumOfEntries(); id++) {
    auto& entry = keeper.getEntry(id);

    // Add nodes
    addProcNodes(entry, *data_procs.at(id), *hist_procs.at(id), ana_procs.at(id));
    addDataNodes(entry);

    // Connect nodes
    connect(procFE.back().get(), rawData.back().get());
    connect(rawData.back().get(), procData.back().get());
    connect(procData.back().get(), events.back().get());
    connect(events.back().get(), procHist.back().get());
    connect(procHist.back().get(), hists.back().get());
    connect(hists.back().get(), procAna.front().back().get());

    assert(procAna.size()==results.size());

    for (unsigned i = 0; i < procAna.size(); i++) {
      if (i > 0) { // connect to upstream if not the first procAna
        connect(results[i-1].back().get(), procAna[i].back().get());
      }

      // connect downstream
      connect(procAna[i].back().get(), results[i].back().get());
    }
  }

  setCoordinates();
  setConnections();
}

void YarrDiagram::addProcNodes(
  const BookEntry& entry,
  const FeDataProcessor& dataproc,
  const HistoDataProcessor& histogrammer,
  const std::vector<std::unique_ptr<DataProcessor>>& analyses
) {
  // Add front end node
  procFE.push_back(std::make_unique<Box>());
  procFE.back()->label = typeid(*entry.fe).name();
  procFE.back()->label_left = "tx="+std::to_string(entry.txChannel)+"\n"+"rx="+std::to_string(entry.rxChannel);
  procFE.back()->objPtr = entry.fe;
  if (procFE.back()->width > maxCellWidth)
    maxCellWidth = procFE.back()->width;

  // Add data processor node
  procData.push_back(std::make_unique<Box>());
  procData.back()->label = typeid(dataproc).name();
  procData.back()->objPtr = &dataproc;
  if (procData.back()->width > maxCellWidth)
    maxCellWidth = procData.back()->width;

  // Add histogrammer node
  procHist.push_back(std::make_unique<Box>());
  procHist.back()->label = typeid(histogrammer).name();
  procHist.back()->objPtr = &histogrammer;
  if (procHist.back()->width > maxCellWidth)
    maxCellWidth = procHist.back()->width;

  // Add analysis nodes
  // procAna: the inner vector is for different rows (ie. FEs) of the same column;
  // The outer vector is for different columns
  if (procAna.size() < analyses.size()) {
    procAna.resize(analyses.size());
  }

  for (unsigned ia = 0; ia < analyses.size(); ia++) {
    procAna[ia].push_back(std::make_unique<Box>());
    auto anaPtr = analyses[ia].get();
    procAna[ia].back()->label = typeid(*anaPtr).name();
    procAna[ia].back()->objPtr = analyses[ia].get();
    if (procAna[ia].back()->width > maxCellWidth)
      maxCellWidth = procAna[ia].back()->width;
  }
}

void YarrDiagram::addDataNodes(const BookEntry& entry) {
  rawData.push_back(std::make_unique<Hex>());
  rawData.back()->label = typeid(entry.fe->clipRawData).name();
  rawData.back()->objPtr = &(entry.fe->clipRawData);
  if (rawData.back()->width > maxCellWidth)
    maxCellWidth = rawData.back()->width;

  events.push_back(std::make_unique<Hex>());
  events.back()->label = typeid(entry.fe->clipData).name();
  events.back()->objPtr = &(entry.fe->clipData);
  if (events.back()->width > maxCellWidth)
    maxCellWidth = events.back()->width;

  hists.push_back(std::make_unique<Hex>());
  hists.back()->label = typeid(entry.fe->clipHisto).name();
  hists.back()->objPtr = &(entry.fe->clipHisto);
  if (hists.back()->width > maxCellWidth)
    maxCellWidth = hists.back()->width;

  // results: the inner vector is for different rows (FEs) of the same column;
  // The outer vector is for different columns
  if (results.size() < entry.fe->clipResult.size()) {
    results.resize(entry.fe->clipResult.size());
  }

  for (unsigned i = 0; i < entry.fe->clipResult.size(); i++) {
    results[i].push_back(std::make_unique<Hex>());
    auto resPtr = entry.fe->clipResult[i].get();
    results[i].back()->label = typeid(*resPtr).name();
    results[i].back()->objPtr = (entry.fe->clipResult)[i].get();
    if (results[i].back()->width > maxCellWidth)
      maxCellWidth = results[i].back()->width;
  }
}

void YarrDiagram::getStats() {
    using RawDataType = const ClipBoard<RawDataContainer>;
    using EventType = const ClipBoard<EventDataBase>;
    using HistType = const ClipBoard<HistogramBase>;
    using ResultType = const ClipBoard<HistogramBase>;

    for (auto& node : rawData) {
      // Add the number of input data objects to the left label of the node
      auto datain = static_cast<RawDataType*>(node->objPtr)->getNumDataIn();
      node->label_left = std::to_string(datain);

      // Add the number of output data objects to the right label of the node
      auto dataout = static_cast<RawDataType*>(node->objPtr)->getNumDataOut();
      node->label_right = std::to_string(dataout);
    }

    for (auto& node : events) {
      // Add the number of input data objects to the left label of the node
      auto datain = static_cast<EventType*>(node->objPtr)->getNumDataIn();
      node->label_left = std::to_string(datain);

      // Add the number of output data objects to the right label of the node
      auto dataout = static_cast<EventType*>(node->objPtr)->getNumDataOut();
      node->label_right = std::to_string(dataout);
    }

    for (auto& node : hists) {
      // Add the number of input data objects to the left label of the node
      auto datain = static_cast<HistType*>(node->objPtr)->getNumDataIn();
      node->label_left = std::to_string(datain);

      // Add the number of output data objects to the right label of the node
      auto dataout = static_cast<HistType*>(node->objPtr)->getNumDataOut();
      node->label_right = std::to_string(dataout);
    }
    for (const auto& res : results) {
      for (auto& node : res) {
        // Add the number of input data objects to the left label of the node
        auto datain = static_cast<ResultType*>(node->objPtr)->getNumDataIn();
        node->label_left = std::to_string(datain);

        // Add the number of output data objects to the right label of the node
        auto dataout = static_cast<ResultType*>(node->objPtr)->getNumDataOut();
        node->label_right = std::to_string(dataout);
      }
    }
}

void YarrDiagram::connect(Node* up, Node* down) {
  up->downstreams.push_back(down);
  down->upstreams.push_back(up);
}

void YarrDiagram::setCoordinates() {
  // cell positions
  float pos_x, pos_y;

  // x positions of edges
  float x0 = 0, x1 = 0;

  // max number of rows
  unsigned nrows = 0;

  auto setPositionColumn = [&](std::vector<std::unique_ptr<Node>>& nodes) {
    // The previous right edge becomes the current left edge
    x0 = x1;
    // The new right edge (assume all nodes in the same column are of the same length)
    x1 = x0 + margin_left + nodes.back()->length + margin_right;

    pos_x = (x0 + x1) / 2;

    // loop over rows
    for (unsigned irow = 0; irow < nodes.size(); irow++) {
      pos_y = (irow + 0.5) * (margin_top + maxCellWidth + margin_bottom);
      nodes[irow]->setPosition(pos_x, pos_y);
    }

    if (nodes.size() > nrows) nrows = nodes.size();
  };

  // procFE -> rawData -> procData -> events -> procHist -> hists -> procAna -> results
  setPositionColumn(procFE);
  setPositionColumn(rawData);
  setPositionColumn(procData);
  setPositionColumn(events);
  setPositionColumn(procHist);
  setPositionColumn(hists);

  assert(procAna.size()==results.size());
  for (unsigned i=0; i < procAna.size(); i++) {
    setPositionColumn(procAna[i]);
    setPositionColumn(results[i]);
  }

  totalLength = x1;
  totalWidth = nrows * (margin_top + maxCellWidth + margin_bottom);
}

void YarrDiagram::setConnections() {
  auto makeLines = [this](const std::vector<std::unique_ptr<Node>>& nodes) {
    for (const auto& node : nodes) {
      for (Node* up : node->upstreams) {
        connections.emplace_back();
        connections.back().start = {up->position.first + up->length / 2, up->position.second};
        connections.back().end = {node->position.first - node->length / 2, node->position.second};
      }

      for (Node* down : node->downstreams) {
        connections.emplace_back();
        connections.back().start = {node->position.first + node->length / 2, node->position.second};
        connections.back().end = {down->position.first - down->length / 2, down->position.second};
      }
    }
  };

  // Only need to consider either data containers or data processors
  makeLines(rawData);
  makeLines(events);
  makeLines(hists);
  for (const auto& res : results) {
    makeLines(res);
  }
}

void YarrDiagram::toJson(json &j) {
  j["Length"] = totalLength;
  j["Width"] = totalWidth;
  j["Diagram"] = json::array();

  unsigned nblocks = 0;

  // Processors
  for (const auto& node : procFE) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& node : procData) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& node : procHist) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& anas : procAna) {
    for (const auto& node : anas) {
      node->toJson(j["Diagram"][nblocks++]);
    }
  }

  // Data containers
  for (const auto& node : rawData) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& node : events) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& node : hists) {
    node->toJson(j["Diagram"][nblocks++]);
  }

  for (const auto& res : results) {
    for (const auto& node : res) {
      node->toJson(j["Diagram"][nblocks++]);
    }
  }

  // Connections
  for (const auto& line : connections) {
    line.toJson(j["Diagram"][nblocks++]);
  }
}

void YarrDiagram::toFile(const std::string& filename) {
  std::ofstream fdiagram(filename);
  json jdiagram;
  this->toJson(jdiagram);
  fdiagram << std::setw(4) << jdiagram;
  fdiagram.close();
}

void YarrDiagram::toPlot(const std::string& filename) {

}
