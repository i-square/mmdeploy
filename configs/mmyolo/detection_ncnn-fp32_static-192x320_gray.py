_base_ = ['./base_static.py']

codebase_config = dict(model_type='ncnn_end2end')

# here is WxH
onnx_config = dict(
    input_shape=[192, 320], output_names=['detection_output'])

backend_config = dict(type='ncnn', precision='FP32', use_vulkan=True)
