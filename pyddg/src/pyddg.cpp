
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ddgsolver/ddgsolver.h"

/// this is for the visualization python wrapper
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/curve_network.h"

#include <geometrycentral/surface/surface_mesh.h>
#include <geometrycentral/surface/rich_surface_mesh_data.h>

#include "ddgsolver/force.h"


namespace ddgsolver {
namespace py = pybind11;

int visualizePly(std::string fileName) {
  /// initialize mesh and vpg 
  std::unique_ptr<gcs::HalfedgeMesh> ptrmesh;
  std::unique_ptr<gcs::VertexPositionGeometry> ptrvpg;

  std::tie(ptrmesh, ptrvpg) = gcs::loadMesh(fileName);
  auto& mesh = *ptrmesh;
  auto& vpg = *ptrvpg;

  polyscope::init();
  polyscope::registerCurveNetwork("myNetwork",
    ptrvpg->inputVertexPositions,
    ptrmesh->getFaceVertexList());

  /// get mean curvature 
  //ddgsolver::Force f(mesh, vpg);
  //double Kb = 1.0; //  both of them does not matter
  //double H0 = 0; // for the calculation of curvature.
  //f.getBendingForces(Kb, H0);
  //std::vector<double> xC(f.Hn.rows());
  //for (size_t i = 0; i < f.Hn.rows(); i++) {
  //  xC[i] = f.Hn.row(i)[0] / f.vertexAreaGradientNormal.row(i)[0]; // (use the x coordinate as sample data)
  //}
  //polyscope::getCurveNetwork("myNetwork")->addNodeScalarQuantity("mean curvature", xC);

  polyscope::show();
  return 0;
}

// Initialize the `pyddg` module
PYBIND11_MODULE(pyddg, pyddg) {
  pyddg.doc() = "Python wrapper around the DDG solver C++ library.";

  pyddg.def("driver", &driver, " a driver function",
            py::arg("inputMesh"), py::arg("Kb"), py::arg("H0"), 
            py::arg("Kse"), py::arg("Ksl"), py::arg("Ksg"),
            py::arg("Kv"), py::arg("Vt"), py::arg("gamma"), 
            py::arg("kt"), py::arg("ptInd"), py::arg("extF"), 
            py::arg("conc"), py::arg("h"), py::arg("T"), 
            py::arg("eps"), py::arg("tSave"), py::arg("outputDir"),
            R"delim(
               Run the driver.

               Args:
                   filename (:py:class:`str`): Filename to read.
            
               Returns:
                   :py:class:`int`: success.
            )delim");

  pyddg.def("visualizePly", &visualizePly, " a visualization function",
    py::arg("fileName"));

  pyddg.def("genIcosphere", &genIcosphere, "Generate a icosphere .ply file",
            py::arg("nSub"), py::arg("path"));
};
} // namespace ddgsolver
