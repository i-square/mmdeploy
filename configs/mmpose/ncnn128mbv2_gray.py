#_base_ = ['./pose-detection_static.py', '../_base_/backends/ncnn-int8.py']
_base_ = ['./pose-detection_static.py']

onnx_config = dict(input_shape=[128, 128])

backend_config = dict(type='ncnn', precision='FP32', use_vulkan=True)
