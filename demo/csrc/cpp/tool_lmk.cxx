
#include "mmdeploy/pose_detector.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "arl_timer.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "utils/argparse.h"

DEFINE_ARG_string(model, "Model path");
// DEFINE_ARG_string(image, "Input image path");
DEFINE_ARG_string(image_list, "Input image list path");
// DEFINE_ARG_int32(ch, "channels");
DEFINE_string(output_path, "output_lmk.txt", "output path");
DEFINE_string(device, "cpu", R"(Device name, e.g. "cpu", "cuda")");

int process_one_img(mmdeploy_pose_detector_t& detector, const std::string& img_path,
                    const mmdeploy_rect_t& box, std::vector<float>& lmks, Timer& timer,
                    std::vector<double>& t_imread, std::vector<double>& t_detect) {
  timer.start();
  cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "failed to load image: %s\n", img_path.c_str());
    return -1;
  }
  // 成功了才计时
  auto t = timer.stop_delta<Timer::ms>();
  t_imread.push_back(t);

  // landmark
  timer.start();
  mmdeploy_mat_t img_m{
      (uint8_t*)(img.data),    img.rows, img.cols, 1, MMDEPLOY_PIXEL_FORMAT_GRAYSCALE,
      MMDEPLOY_DATA_TYPE_UINT8};

  mmdeploy_pose_detection_t* res{};
  int face_n = 1;
  int status = mmdeploy_pose_detector_apply_bbox(detector, &img_m, 1, &box, &face_n, &res);

  std::shared_ptr<mmdeploy_pose_detection_t> pose_data(
      res, [](auto p) { mmdeploy_pose_detector_release_result(p, 1); });

  if (status != MMDEPLOY_SUCCESS) {
    fprintf(stderr, "failed to run pose, code: %d\n", (int)status);
    return -2;
  }
  // 成功了才计时
  t = timer.stop_delta<Timer::ms>();
  t_detect.push_back(t);

  for (int i = 0; i < res->length; ++i) {
    lmks.push_back(res->point[i].x);
    lmks.push_back(res->point[i].y);
  }

  return 0;
}

int main(int argc, char* argv[]) {
  if (!utils::ParseArguments(argc, argv)) {
    return -1;
  }

  Timer timer;

  mmdeploy_pose_detector_t detector;
  int status =
      mmdeploy_pose_detector_create_by_path(ARGS_model.c_str(), FLAGS_device.c_str(), 0, &detector);
  if (status != MMDEPLOY_SUCCESS) {
    fprintf(stderr, "failed to create pose detector, code: %d\n", (int)status);
    return -1;
  }

  std::ifstream ifs(ARGS_image_list);
  if (!ifs) {
    fprintf(stderr, "failed to read image_list: %s\n", ARGS_image_list.c_str());
    return -2;
  }
  std::ofstream ofs(FLAGS_output_path);
  if (!ofs) {
    fprintf(stderr, "failed to create output file: %s\n", FLAGS_output_path.c_str());
    return -3;
  }

  std::vector<double> t_imread;
  std::vector<double> t_detect;
  std::string img_path;
  int x, y, w, h;
  float score;
  while (ifs >> img_path >> x >> y >> w >> h >> score) {
    if (w <= 0 || h <= 0) {
      fprintf(stderr, "image no box, path: %s\n", img_path.c_str());
      continue;
    }
    mmdeploy_rect_t box;
    box.left = x;
    box.top = y;
    box.right = x + w;
    box.bottom = y + h;
    std::vector<float> lmks;
    process_one_img(detector, img_path, box, lmks, timer, t_imread, t_detect);
    ofs << img_path << " " << x << " " << y << " " << w << " " << h << " " << score;
    for (auto& v : lmks) {
      ofs << " " << v;
    }
    ofs << std::endl;
  }
  // timer
  auto calc_mean = [](std::vector<double>& v) {
    if (v.empty()) {
      return 0.0;
    }
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    return sum / v.size();
  };
  auto mean_imread = calc_mean(t_imread);
  auto mean_detect = calc_mean(t_detect);
  fprintf(stdout,
          "process done, imread: %lu avg cost(ms): %lf, pose_detect: %lu avg cost(ms): %lf\n",
          t_imread.size(), mean_imread, t_detect.size(), mean_detect);

  return 0;
}
