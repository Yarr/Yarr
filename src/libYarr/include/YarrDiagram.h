#ifndef YARR_DIAGRAM_H
#define YARR_DIAGRAM_H

#include <string>
#include <vector>

#include "Bookkeeper.h"
#include "DataProcessor.h"
#include "FeDataProcessor.h"
#include "HistoDataProcessor.h"

class Node;
class Line;

class YarrDiagram {

  public:

    YarrDiagram() = default;
    ~YarrDiagram() = default;

    void makeDiagram(
      Bookkeeper& keeper,
      const std::map<unsigned, std::unique_ptr<FeDataProcessor>>& data_procs,
      const std::map<unsigned, std::unique_ptr<HistoDataProcessor>>& hist_procs,
      const std::map<unsigned, std::vector<std::unique_ptr<DataProcessor>>>& ana_procs
    );

    void getStats();
    void toJson(json& j);
    void toFile(const std::string& filename);
    void toPlot(const std::string& filename);

  private:

    void addProcNodes(const BookEntry&, const FeDataProcessor&, const HistoDataProcessor&, const std::vector<std::unique_ptr<DataProcessor>>&);

    void addDataNodes(const BookEntry&);

    void connect(Node*, Node*);

    void setCoordinates();
    void setConnections();

    // Data processors
    std::vector<std::unique_ptr<Node>> procFE;
    std::vector<std::unique_ptr<Node>> procData;
    std::vector<std::unique_ptr<Node>> procHist;
    std::vector<std::vector<std::unique_ptr<Node>>> procAna;

    // Data containers
    std::vector<std::unique_ptr<Node>> rawData;
    std::vector<std::unique_ptr<Node>> events;
    std::vector<std::unique_ptr<Node>> hists;
    std::vector<std::vector<std::unique_ptr<Node>>> results;

    // Connections
    std::vector<Line> connections;

    // size
    float maxCellWidth {0};
    float totalLength {0};
    float totalWidth {0};

    // margins
    float margin_top {0.5};
    float margin_bottom {0.5};
    float margin_left {1.5};
    float margin_right {0.5};
};

struct Node {

  virtual ~Node() = default;

  // connections
  std::vector<Node*> upstreams;
  std::vector<Node*> downstreams;

  // labels
  std::string label;
  std::string label_left;
  std::string label_right;

  // position
  std::pair<float,float> position;

  // size
  float length {4};
  float width {1};

  const void* objPtr;

  virtual void setPosition(float, float) = 0;

  virtual void toJson(json& j) const {
    if (not label.empty())
      j["label"] = label;

    if (not label_left.empty())
      j["label_left"] = label_left;

    if (not label_right.empty())
      j["label_right"] = label_right;
  }
};

// Rectangle
struct Box : Node {

  float x0;
  float x1;
  float y0;
  float y1;

  void setPosition(float x, float y) override {
    position = {x, y};

    x0 = x - length/2;
    x1 = x + length/2;
    y0 = y - width/2;
    y1 = y + width/2;
  }

  void toJson(json& j) const override {
    j["shape"] = "rectangle";
    j["x"] = {x0, x1};
    j["y"] = {y0, y1};

    // Add labels
    Node::toJson(j);
  }

};

// Hexagon
struct Hex : Node {

  std::vector<std::pair<float, float>> vertices;

  void setPosition(float x, float y) override {
    position = {x , y};

    vertices.clear();
    vertices.emplace_back(position.first - 0.45*length, position.second - 0.5*width);
    vertices.emplace_back(position.first - 0.50*length, position.second);
    vertices.emplace_back(position.first - 0.45*length, position.second + 0.5*width);
    vertices.emplace_back(position.first + 0.45*length, position.second + 0.5*width);
    vertices.emplace_back(position.first + 0.50*length, position.second);
    vertices.emplace_back(position.first + 0.45*length, position.second - 0.5*width);
    vertices.emplace_back(position.first - 0.45*length, position.second - 0.5*width);
  }

  void toJson(json& j) const override {

    j["shape"] = "hexagon";
    j["x"] = json::array();
    j["y"] = json::array();
    for (unsigned v = 0; v < vertices.size(); v++) {
      j["x"][v] = vertices[v].first;
      j["y"][v] = vertices[v].second;
    }

    // Add labels
    Node::toJson(j);
  }

};

// Straing Line
struct Line {
  std::pair<float, float> start;
  std::pair<float, float> end;

  void toJson(json& j) const {
    j["shape"] = "line";
    j["x"] = {start.first, end.first};
    j["y"] = {start.second, end.second};
  }
};

#endif
