#include "mmdeploy/detector.hpp"

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
DEFINE_string(output_path, "output_det.txt", "output path");
DEFINE_string(device, "cpu", R"(Device name, e.g. "cpu", "cuda")");
// DEFINE_string(output, "output_detection.png", "Output image path");

// DEFINE_double(det_thr, .1, "Detection score threshold");

struct Box {
  int x = -1;
  int y = -1;
  int w = -1;
  int h = -1;
  float score = -1.0f;
};

int process_one_img(mmdeploy_detector_t& detector, const std::string& img_path, Box& box,
                    Timer& timer, std::vector<double>& t_imread, std::vector<double>& t_detect) {
  timer.start();
  cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
  if (img.empty()) {
    fprintf(stderr, "failed to load image: %s\n", img_path.c_str());
    return -1;
  }
  // 成功了才计时
  auto t = timer.stop_delta<Timer::ms>();
  t_imread.push_back(t);

  // detect
  timer.start();
  mmdeploy_mat_t img_m{
      (uint8_t*)(img.data),    img.rows, img.cols, 1, MMDEPLOY_PIXEL_FORMAT_GRAYSCALE,
      MMDEPLOY_DATA_TYPE_UINT8};

  mmdeploy_detection_t* results{};
  int* result_count{};

  int status = mmdeploy_detector_apply(detector, &img_m, 1, &results, &result_count);

  std::shared_ptr<mmdeploy_detection_t> detect_data(
      results, [result_count](auto p) { mmdeploy_detector_release_result(p, result_count, 1); });

  if (status != MMDEPLOY_SUCCESS) {
    fprintf(stderr, "failed to detect, code: %d\n", (int)status);
    return -2;
  }
  // 成功了才计时
  t = timer.stop_delta<Timer::ms>();
  t_detect.push_back(t);

  auto face_n = *result_count;
  if (face_n == 0) {
    box.x = 0;
    box.y = 0;
    box.w = 0;
    box.h = 0;
    box.score = 0.0f;
  } else {
    // 只取第一个结果
    auto& bbox = results[0].bbox;
    box.x = bbox.left;
    box.y = bbox.top;
    box.w = bbox.right - bbox.left;
    box.h = bbox.bottom - bbox.top;
    box.score = results[0].score;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (!utils::ParseArguments(argc, argv)) {
    return -1;
  }

  Timer timer;

  mmdeploy_detector_t detector;
  int status =
      mmdeploy_detector_create_by_path(ARGS_model.c_str(), FLAGS_device.c_str(), 0, &detector);
  if (status != MMDEPLOY_SUCCESS) {
    fprintf(stderr, "failed to create detector, code: %d\n", (int)status);
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
  while (ifs >> img_path) {
    Box box;
    process_one_img(detector, img_path, box, timer, t_imread, t_detect);
    ofs << img_path << " " << box.x << " " << box.y << " " << box.w << " " << box.h << " "
        << box.score << std::endl;
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
  fprintf(stdout, "process done, imread: %lu avg cost(ms): %lf, detect: %lu avg cost(ms): %lf\n",
          t_imread.size(), mean_imread, t_detect.size(), mean_detect);

  return 0;
}
