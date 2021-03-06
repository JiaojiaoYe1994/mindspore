/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/ops/primitive_c.h"
#include <memory>
#include <map>
#include "src/ops/space_to_batch.h"
#include "src/ops/space_to_batch_nd.h"
#include "src/ops/conv2d.h"
#include "src/ops/roi_pooling.h"
#include "src/ops/topk.h"
#include "src/ops/broadcast_to.h"
#include "src/ops/unsqueeze.h"
#include "src/ops/unstack.h"
#include "src/ops/depth_to_space.h"
#include "src/ops/batch_to_space.h"
#include "src/ops/prior_box.h"
#include "src/ops/lstm.h"
#include "src/ops/softmax.h"
#include "src/ops/activation.h"
#include "src/ops/deconv2d.h"
#include "src/ops/reduce.h"
#include "src/ops/pooling.h"
#include "src/ops/fused_batchnorm.h"
#include "src/ops/batch_norm.h"
#include "src/ops/power.h"
#include "src/ops/range.h"
#include "src/ops/add.h"
#include "src/ops/sub.h"
#include "src/ops/div.h"
#include "src/ops/bias_add.h"
#include "src/ops/expand_dims.h"
#include "src/ops/full_connection.h"
#include "src/ops/shape.h"
#include "src/ops/elu.h"
#include "src/ops/embedding_lookup.h"
#include "src/ops/quant_dtype_cast.h"
#include "src/ops/matmul.h"
#include "src/ops/resize.h"
#include "src/ops/tile.h"
#include "src/ops/one_hot.h"
#include "src/ops/space_to_depth.h"
#include "src/ops/split.h"
#include "src/ops/argmax.h"
#include "src/ops/argmin.h"
#include "src/ops/cast.h"
#include "src/ops/reshape.h"
#include "src/ops/scale.h"
#include "src/ops/concat.h"
#include "src/ops/nchw2nhwc.h"
#include "src/ops/slice.h"
#include "src/ops/squeeze.h"
#include "src/ops/flatten.h"
#include "src/ops/mean.h"
#include "src/ops/nhwc2nchw.h"
#include "src/ops/stack.h"
#include "src/ops/crop.h"
#include "src/ops/addn.h"
#include "src/ops/gather.h"
#include "src/ops/gather_nd.h"
#include "src/ops/local_response_normalization.h"
#include "src/ops/pad.h"
#include "src/ops/p_relu.h"
#include "src/ops/leaky_relu.h"
#include "src/ops/reverse_sequence.h"
#include "src/ops/dedepthwise_conv2d.h"
#include "src/ops/depthwise_conv2d.h"
#include "src/ops/mul.h"
#include "src/ops/eltwise.h"
#include "src/ops/fill.h"
#include "src/ops/transpose.h"
#include "src/ops/log.h"
#include "src/ops/abs.h"
#include "src/ops/sin.h"
#include "src/ops/cos.h"
#include "src/ops/sqrt.h"
#include "src/ops/square.h"
#include "src/ops/exp.h"
#include "src/ops/rsqrt.h"
#include "src/ops/maximum.h"
#include "src/ops/minimum.h"
#include "src/ops/strided_slice.h"
#include "src/ops/reverse.h"
#include "src/ops/logical_and.h"
#include "src/ops/logical_or.h"
#include "src/ops/logical_not.h"
#include "src/ops/floor_div.h"
#include "src/ops/floor_mod.h"
#include "src/ops/equal.h"
#include "src/ops/not_equal.h"
#include "src/ops/less.h"
#include "src/ops/less_equal.h"
#include "src/ops/greater_equal.h"
#include "src/ops/greater.h"
#include "src/ops/floor.h"
#include "src/ops/squared_difference.h"
#include "src/ops/ceil.h"
#include "src/ops/round.h"
#include "src/ops/unique.h"
#include "src/ops/zeros_like.h"
#include "src/ops/return.h"
#include "src/ops/where.h"
#include "src/ops/scatter_nd.h"
#include "src/ops/constant_of_shape.h"
#include "src/ops/dequant.h"
#include "src/ops/make_tuple.h"
#include "src/ops/quant.h"
#include "src/ops/tuple_get_item.h"
#include "src/ops/l2_norm.h"
#include "src/ops/neg.h"
#include "src/ops/sparse_to_dense.h"
#include "src/ops/detection_post_process.h"
#include "src/ops/dropout.h"
#include "src/ops/real_div.h"
#ifdef PRIMITIVE_WRITEABLE
#include "tools/converter/quantizer/quantize_util.h"
#endif

#ifdef SUPPORT_TRAIN
#include "src/ops/neg_grad.h"
#include "src/ops/activation_grad.h"
#include "src/ops/apply_momentum.h"
#include "src/ops/bias_grad.h"
#include "src/ops/pooling_grad.h"
#include "src/ops/conv2d_grad_filter.h"
#include "src/ops/conv2d_grad_input.h"
#include "src/ops/power_grad.h"
#include "src/ops/softmax_cross_entropy.h"
#include "src/ops/bn_grad.h"
#include "src/ops/arithmetic_grad.h"
#include "src/ops/depend.h"
#include "src/ops/flatten_grad.h"
#include "src/ops/log_grad.h"
#endif

namespace mindspore {
namespace lite {
#ifdef PRIMITIVE_WRITEABLE
void PrimitiveC::CalQuantParam(const double &mean, const double &stdDev, float *mMin, float *mMax) {
  const float qmin = 0;
  const float qmax = 255;
  *mMin = static_cast<float>((qmin - mean) / stdDev);
  *mMax = static_cast<float>((qmax - mean) / stdDev);
}

void PrimitiveC::PopulaterQuantParam(const Primitive &prim,
                                     std::vector<std::vector<schema::QuantParamT>> *vecInputQuantParam,
                                     std::vector<std::vector<schema::QuantParamT>> *vecOutputQuantParam,
                                     const std::vector<AnfNodePtr> &inputs) {
  auto narrow_range = prim.GetAttr("narrow_range");
  bool narrowRangeQuantParam = GetValue<bool>(narrow_range);
  auto num_bits = prim.GetAttr("num_bits");
  int32_t numbitsRangeQuantParam = GetValue<int32_t>(num_bits);

  std::vector<schema::QuantParamT> quants;
  schema::QuantParamT quantParam;
  auto mean = prim.GetAttr("mean");
  auto std_dev = prim.GetAttr("std_dev");
  if (mean != nullptr && std_dev != nullptr) {
    auto meanQuantOaram = GetValue<double>(mean);
    double stddevQuantOaram = GetValue<double>(std_dev);
    float mMin = 0.0;
    float mMax = 0.0;
    CalQuantParam(meanQuantOaram, stddevQuantOaram, &mMin, &mMax);
    quantParam.min = mMin;
    quantParam.max = mMax;
  } else {
    auto inputMin = prim.GetAttr("input_minq");
    auto inputMax = prim.GetAttr("input_maxq");
    if (inputMin != nullptr && inputMax != nullptr) {
      auto inputMinPtr = inputMin->cast<TensorPtr>();
      auto inputMaxPtr = inputMax->cast<TensorPtr>();
      float *minBuf = static_cast<float *>(inputMinPtr->data_c());
      float *maxBuf = static_cast<float *>(inputMaxPtr->data_c());
      quantParam.min = *minBuf;
      quantParam.max = *maxBuf;
    }
  }
  quant::CalQuantizationParams(&quantParam, quantParam.min, quantParam.max, narrowRangeQuantParam,
                               numbitsRangeQuantParam);
  quants.emplace_back(quantParam);
  vecInputQuantParam->emplace_back(quants);

  quants.clear();
  auto filterMin = prim.GetAttr("filter_minq");
  auto filterMax = prim.GetAttr("filter_maxq");
  if (filterMin != nullptr && filterMax != nullptr) {
    auto filterMinPtr = filterMin->cast<TensorPtr>();
    auto filterMaxPtr = filterMax->cast<TensorPtr>();
    float *minBuf = static_cast<float *>(filterMinPtr->data_c());
    float *maxBuf = static_cast<float *>(filterMaxPtr->data_c());
    quantParam.min = FLT_MAX;
    quantParam.max = FLT_MIN;
    for (int i = 0; i < filterMinPtr->ElementsNum(); ++i) {
      quantParam.min = (*(minBuf) < quantParam.min) ? (*minBuf) : quantParam.min;
      quantParam.max = (*(maxBuf) > quantParam.max) ? (*maxBuf) : quantParam.max;
      minBuf++;
      maxBuf++;
    }
    quant::CalQuantizationParams(&quantParam, quantParam.min, quantParam.max, true, numbitsRangeQuantParam);
    quants.emplace_back(quantParam);
    vecInputQuantParam->emplace_back(quants);
  }

  if (vecInputQuantParam->size() == kDoubleNum) {
    quants.clear();
    quantParam.min = 0.0;
    quantParam.max = 0.0;
    quantParam.zeroPoint = 0;
    quantParam.scale = vecInputQuantParam->at(0).at(0).scale * vecInputQuantParam->at(1).at(0).scale;
    quants.emplace_back(quantParam);
    vecInputQuantParam->emplace_back(quants);
  }

  quants.clear();
  auto outputMin = prim.GetAttr("output_minq");
  auto outputMax = prim.GetAttr("output_maxq");
  if (outputMin != nullptr && outputMax != nullptr) {
    auto outputMinPtr = outputMin->cast<TensorPtr>();
    auto outputMaxPtr = outputMax->cast<TensorPtr>();
    float *minBuf = static_cast<float *>(outputMinPtr->data_c());
    float *maxBuf = static_cast<float *>(outputMaxPtr->data_c());
    quantParam.min = *minBuf;
    quantParam.max = *maxBuf;
    quant::CalQuantizationParams(&quantParam, quantParam.min, quantParam.max, narrowRangeQuantParam,
                                 numbitsRangeQuantParam);
    quants.emplace_back(quantParam);
    vecOutputQuantParam->emplace_back(quants);
  }
}

void PrimitiveC::GetAttrDataFromInput(const AnfNodePtr inputNode, std::vector<int> *data) {
  if (inputNode->isa<ValueNode>()) {
    auto valNode = inputNode->cast<ValueNodePtr>();
    MS_ASSERT(valNode != nullptr);
    auto val = valNode->value();
    MS_ASSERT(val != nullptr);
    if (val->isa<ValueTuple>()) {
      auto tuple = val->cast<ValueTuplePtr>();
      MS_ASSERT(tuple != nullptr);
      for (size_t i = 0; i < tuple->size(); i++) {
        auto elem = tuple->value()[i]->cast<Int32ImmPtr>();
        MS_ASSERT(elem != nullptr);
        data->emplace_back(static_cast<int>(elem->value()));
      }
    }
  }
}

schema::PrimitiveT *PrimitiveC::GetPrimitiveT() const { return this->primitive_; }

void PrimitiveC::ClearPrimitiveT() { this->primitive_ = nullptr; }

void PrimitiveC::SetInputQuantParam(const std::vector<std::vector<schema::QuantParamT>> &input_quant_param) {
  this->input_quant_param_ = input_quant_param;
}

void PrimitiveC::SetOutputQuantParam(const std::vector<std::vector<schema::QuantParamT>> &output_quant_param) {
  this->output_quant_param_ = output_quant_param;
}

void PrimitiveC::ClearInputOutputQuantParam() {
  input_quant_param_.clear();
  output_quant_param_.clear();
}

void PrimitiveC::AddInputQuantParam(std::vector<schema::QuantParamT> quant_param) {
  this->input_quant_param_.emplace_back(quant_param);
}
std::vector<std::vector<schema::QuantParamT>> PrimitiveC::GetInputQuantParams() const { return input_quant_param_; }

void PrimitiveC::AddOutputQuantParam(std::vector<schema::QuantParamT> quant_param) {
  this->output_quant_param_.emplace_back(quant_param);
}
std::vector<std::vector<schema::QuantParamT>> PrimitiveC::GetOutputQuantParams() const { return output_quant_param_; }

void PrimitiveC::SetQuantType(const schema::QuantType &quant_type) { this->quant_type_ = quant_type; }

schema::QuantType PrimitiveC::GetQuantType() const { return quant_type_; }

std::shared_ptr<PrimitiveC> GetReturnPrim() {
  auto return_primitiveT = new (std::nothrow) schema::PrimitiveT;
  if (return_primitiveT == nullptr) {
    MS_LOG(ERROR) << "new PrimitiveT failed";
    return nullptr;
  }
  return_primitiveT->value.type = schema::PrimitiveType_Return;
  return_primitiveT->value.value = new schema::ReturnT;
  if (return_primitiveT->value.value == nullptr) {
    MS_LOG(ERROR) << "new ReturnT failed";
    delete (return_primitiveT);
    return nullptr;
  }
  return std::make_shared<Return>(return_primitiveT);
}

std::shared_ptr<PrimitiveC> GetMakeTuplePrim() {
  auto make_tuple_primitiveT = new schema::PrimitiveT;
  if (make_tuple_primitiveT == nullptr) {
    MS_LOG(ERROR) << "new PrimitiveT failed";
    return nullptr;
  }
  make_tuple_primitiveT->value.type = schema::PrimitiveType_MakeTuple;
  make_tuple_primitiveT->value.value = new schema::MakeTupleT;
  if (make_tuple_primitiveT->value.value == nullptr) {
    MS_LOG(ERROR) << "new MakeTupleT failed";
    delete (make_tuple_primitiveT);
    return nullptr;
  }
  return std::make_shared<MakeTuple>(make_tuple_primitiveT);
}

std::shared_ptr<PrimitiveC> GetTupleGetItemPrim() {
  auto tuple_get_item_primitiveT = new schema::PrimitiveT();
  if (tuple_get_item_primitiveT == nullptr) {
    MS_LOG(ERROR) << "new PrimitiveT failed";
    return nullptr;
  }
  tuple_get_item_primitiveT->value.type = schema::PrimitiveType_TupleGetItem;
  tuple_get_item_primitiveT->value.value = new schema::TupleGetItemT;
  if (tuple_get_item_primitiveT->value.value == nullptr) {
    MS_LOG(ERROR) << "new TupleGetItemT failed";
    delete (tuple_get_item_primitiveT);
    return nullptr;
  }
  return std::make_shared<TupleGetItem>(tuple_get_item_primitiveT);
}

template <typename T, typename = std::enable_if<std::is_base_of<PrimitiveC, T>::value>>
std::shared_ptr<PrimitiveC> NewPrimitiveC(const Primitive &prim, const std::vector<AnfNodePtr> &inputs,
                                          const schema::QuantType &quantType) {
  auto primc = std::make_shared<T>();
  if (primc == nullptr) {
    MS_LOG(ERROR) << "make_shared PrimitiveC failed";
    return nullptr;
  }
  primc->SetQuantType(quantType);
  auto ret = primc->UnPackAttr(prim, inputs);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "UnPackAttr failed";
    return nullptr;
  }
  return primc;
}

std::shared_ptr<PrimitiveC> PrimitiveC::Create(const Primitive &prim, const std::vector<AnfNodePtr> &inputs,
                                               const schema::QuantType &quantType) {
  const auto &op_type = prim.name();
  if (op_type == "ReLU" || op_type == "ReLU6" || op_type == "Sigmoid") {
    return NewPrimitiveC<Activation>(prim, inputs, quantType);
  } else if (op_type == "AddN") {
    return NewPrimitiveC<AddN>(prim, inputs, quantType);
  } else if (op_type == "BatchNorm") {
    return NewPrimitiveC<BatchNorm>(prim, inputs, quantType);
  } else if (op_type == "BiasAdd") {
    return NewPrimitiveC<BiasAdd>(prim, inputs, quantType);
  } else if (op_type == "Concat") {
    return NewPrimitiveC<Concat>(prim, inputs, quantType);
  } else if (op_type == "Conv2D") {
    return NewPrimitiveC<Conv2D>(prim, inputs, quantType);
  } else if (op_type == "DepthwiseConv2dNative" || op_type == "DepthwiseConv2D") {
    return NewPrimitiveC<DepthwiseConv2D>(prim, inputs, quantType);
  } else if (op_type == "Dequant") {
    return NewPrimitiveC<Dequant>(prim, inputs, quantType);
  } else if (op_type == "Flatten") {
    return NewPrimitiveC<Flatten>(prim, inputs, quantType);
  } else if (op_type == "FusedBatchNorm") {
    return NewPrimitiveC<FusedBatchNorm>(prim, inputs, quantType);
  } else if (op_type == "make_tuple") {
    return NewPrimitiveC<MakeTuple>(prim, inputs, quantType);
  } else if (op_type == "MatMul") {
    return NewPrimitiveC<MatMul>(prim, inputs, quantType);
  } else if (op_type == "Mul") {
    return NewPrimitiveC<Mul>(prim, inputs, quantType);
  } else if (op_type == "MaxPool" || op_type == "AvgPool") {
    return NewPrimitiveC<Pooling>(prim, inputs, quantType);
  } else if (op_type == "Quant") {
    return NewPrimitiveC<Quant>(prim, inputs, quantType);
  } else if (op_type == "RealDiv") {
    return NewPrimitiveC<RealDiv>(prim, inputs, quantType);
  } else if (op_type == "ReduceMax") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "ReduceMean") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "ReduceMin") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "ReduceProd") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "ReduceSum") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "ReduceSumSquare") {
    return NewPrimitiveC<Reduce>(prim, inputs, quantType);
  } else if (op_type == "Reshape") {
    return NewPrimitiveC<Reshape>(prim, inputs, quantType);
  } else if (op_type == "TensorAdd") {
    return NewPrimitiveC<Add>(prim, inputs, quantType);
  } else if (op_type == "Transpose") {
    return NewPrimitiveC<Transpose>(prim, inputs, quantType);
  } else if (op_type == "Elu") {
    return NewPrimitiveC<Elu>(prim, inputs, quantType);
  } else if (op_type == "Log") {
    return NewPrimitiveC<Log>(prim, inputs, quantType);
  } else if (op_type == "DeConv2D") {
    return NewPrimitiveC<DeConv2D>(prim, inputs, quantType);
  } else if (op_type == "tuple_getitem") {
    return NewPrimitiveC<TupleGetItem>(prim, inputs, quantType);
  } else if (op_type == "Softmax") {
    return NewPrimitiveC<SoftMax>(prim, inputs, quantType);
  } else if (op_type == "StridedSlice") {
    return NewPrimitiveC<StridedSlice>(prim, inputs, quantType);
  } else if (op_type == "Cast") {
    return NewPrimitiveC<Cast>(prim, inputs, quantType);
  } else if (op_type == "Split") {
    return NewPrimitiveC<Split>(prim, inputs, quantType);

#ifdef SUPPORT_TRAIN
  } else if (op_type == "SoftmaxCrossEntropyWithLogits") {
    return NewPrimitiveC<SoftmaxCrossEntropy>(prim, inputs, quantType);
  } else if (op_type == "BiasAddGrad") {
    return NewPrimitiveC<BiasGrad>(prim, inputs, quantType);
  } else if (op_type == "ApplyMomentum") {
    return NewPrimitiveC<ApplyMomentum>(prim, inputs, quantType);
  } else if (op_type == "Depend") {
    return NewPrimitiveC<Depend>(prim, inputs, quantType);
  } else if ((op_type == "ReluGrad" || op_type == "Relu6Grad" || op_type == "SigmoidGrad")) {
    return NewPrimitiveC<ActivationGrad>(prim, inputs, quantType);
  } else if ((op_type == "MaxPoolGrad") || (op_type == "MeanPoolGrad")) {
    return NewPrimitiveC<PoolingGrad>(prim, inputs, quantType);
  } else if (op_type == "Conv2DBackpropFilter") {
    return NewPrimitiveC<Conv2DGradFilter>(prim, inputs, quantType);
  } else if (op_type == "Conv2DBackpropInput") {
    return NewPrimitiveC<Conv2DGradInput>(prim, inputs, quantType);
  } else if (op_type == "BatchNormGrad") {
    return NewPrimitiveC<BNGrad>(prim, inputs, quantType);
  } else if (op_type == "FlattenGrad") {
    return NewPrimitiveC<FlattenGrad>(prim, inputs, quantType);
  } else if (op_type == "FusedBatchNormGrad") {
    return NewPrimitiveC<BNGrad>(prim, inputs, quantType);
  } else if (op_type == "Tile") {
    return NewPrimitiveC<Tile>(prim, inputs, quantType);
#else
  } else if (op_type == "Conv2DBackpropInput") {
    return NewPrimitiveC<DeConv2D>(prim, inputs, quantType);
#endif
  } else {
    MS_LOG(ERROR) << "Unsupported primitive type in Create : " << op_type;
    return nullptr;
  }
}

PrimitiveC *PrimitiveC::Create(mindspore::schema::PrimitiveT *primitive) {
  MS_ASSERT(primitive != nullptr);
  auto op_type = primitive->value.type;
  switch (op_type) {
    case schema::PrimitiveType_SoftMax:
      return new SoftMax(primitive);
    case schema::PrimitiveType_Activation:
      return new Activation(primitive);
    case schema::PrimitiveType_Conv2D:
      return new Conv2D(primitive);
    case schema::PrimitiveType_DeConv2D:
      return new DeConv2D(primitive);
    case schema::PrimitiveType_Reduce:
      return new Reduce(primitive);
    case schema::PrimitiveType_Pooling:
      return new Pooling(primitive);
    case schema::PrimitiveType_ROIPooling:
      return new ROIPooling(primitive);
    case schema::PrimitiveType_DepthwiseConv2D:
      return new DepthwiseConv2D(primitive);
    case schema::PrimitiveType_FusedBatchNorm:
      return new FusedBatchNorm(primitive);
    case schema::PrimitiveType_BatchNorm:
      return new BatchNorm(primitive);
    case schema::PrimitiveType_FullConnection:
      return new FullConnection(primitive);
    case schema::PrimitiveType_Power:
      return new Power(primitive);
    case schema::PrimitiveType_Pad:
      return new Pad(primitive);
    case schema::PrimitiveType_Range:
      return new Range(primitive);
    case schema::PrimitiveType_Mul:
      return new Mul(primitive);
    case schema::PrimitiveType_Add:
      return new Add(primitive);
    case schema::PrimitiveType_Sub:
      return new Sub(primitive);
    case schema::PrimitiveType_Div:
      return new Div(primitive);
    case schema::PrimitiveType_BiasAdd:
      return new BiasAdd(primitive);
    case schema::PrimitiveType_ExpandDims:
      return new ExpandDims(primitive);
    case schema::PrimitiveType_ArgMax:
      return new ArgMax(primitive);
    case schema::PrimitiveType_ArgMin:
      return new ArgMin(primitive);
    case schema::PrimitiveType_Cast:
      return new Cast(primitive);
    case schema::PrimitiveType_Reshape:
      return new Reshape(primitive);
    case schema::PrimitiveType_Scale:
      return new Scale(primitive);
    case schema::PrimitiveType_Eltwise:
      return new Eltwise(primitive);
    case schema::PrimitiveType_Ceil:
      return new Ceil(primitive);
    case schema::PrimitiveType_Concat:
      return new Concat(primitive);
    case schema::PrimitiveType_Fill:
      return new Fill(primitive);
    case schema::PrimitiveType_Nhwc2Nchw:
      return new Nhwc2Nchw(primitive);
    case schema::PrimitiveType_Nchw2Nhwc:
      return new Nchw2Nhwc(primitive);
    case schema::PrimitiveType_Transpose:
      return new Transpose(primitive);
    case schema::PrimitiveType_Slice:
      return new Slice(primitive);
    case schema::PrimitiveType_Squeeze:
      return new Squeeze(primitive);
    case schema::PrimitiveType_Flatten:
      return new Flatten(primitive);
    case schema::PrimitiveType_Mean:
      return new Mean(primitive);
    case schema::PrimitiveType_Stack:
      return new Stack(primitive);
    case schema::PrimitiveType_Crop:
      return new Crop(primitive);
    case schema::PrimitiveType_SquaredDifference:
      return new SquaredDifference(primitive);
    case schema::PrimitiveType_AddN:
      return new AddN(primitive);
    case schema::PrimitiveType_Abs:
      return new Abs(primitive);
    case schema::PrimitiveType_Sin:
      return new Sin(primitive);
    case schema::PrimitiveType_Cos:
      return new Cos(primitive);
    case schema::PrimitiveType_Log:
      return new Log(primitive);
    case schema::PrimitiveType_Sqrt:
      return new Sqrt(primitive);
    case schema::PrimitiveType_Rsqrt:
      return new Rsqrt(primitive);
    case schema::PrimitiveType_Square:
      return new Square(primitive);
    case schema::PrimitiveType_Exp:
      return new Exp(primitive);
    case schema::PrimitiveType_Gather:
      return new Gather(primitive);
    case schema::PrimitiveType_GatherNd:
      return new GatherNd(primitive);
    case schema::PrimitiveType_LocalResponseNormalization:
      return new LocalResponseNormalization(primitive);
    case schema::PrimitiveType_Maximum:
      return new Maximum(primitive);
    case schema::PrimitiveType_Minimum:
      return new Minimum(primitive);
    case schema::PrimitiveType_StridedSlice:
      return new StridedSlice(primitive);
    case schema::PrimitiveType_LeakyReLU:
      return new (std::nothrow) LeakyReLU(primitive);
    case schema::PrimitiveType_PReLU:
      return new (std::nothrow) PReLU(primitive);
    case schema::PrimitiveType_Round:
      return new Round(primitive);
    case schema::PrimitiveType_Reverse:
      return new Reverse(primitive);
    case schema::PrimitiveType_ReverseSequence:
      return new ReverseSequence(primitive);
    case schema::PrimitiveType_LogicalAnd:
      return new LogicalAnd(primitive);
    case schema::PrimitiveType_LogicalOr:
      return new LogicalOr(primitive);
    case schema::PrimitiveType_LogicalNot:
      return new LogicalNot(primitive);
    case schema::PrimitiveType_FloorDiv:
      return new FloorDiv(primitive);
    case schema::PrimitiveType_FloorMod:
      return new FloorMod(primitive);
    case schema::PrimitiveType_Equal:
      return new Equal(primitive);
    case schema::PrimitiveType_NotEqual:
      return new NotEqual(primitive);
    case schema::PrimitiveType_Less:
      return new Less(primitive);
    case schema::PrimitiveType_LessEqual:
      return new LessEqual(primitive);
    case schema::PrimitiveType_Greater:
      return new Greater(primitive);
    case schema::PrimitiveType_GreaterEqual:
      return new GreaterEqual(primitive);
    case schema::PrimitiveType_Floor:
      return new Floor(primitive);
    case schema::PrimitiveType_Split:
      return new Split(primitive);
    case schema::PrimitiveType_OneHot:
      return new OneHot(primitive);
    case schema::PrimitiveType_PriorBox:
      return new PriorBox(primitive);
    case schema::PrimitiveType_SpaceToDepth:
      return new SpaceToDepth(primitive);
    case schema::PrimitiveType_Tile:
      return new Tile(primitive);
    case schema::PrimitiveType_Resize:
      return new Resize(primitive);
    case schema::PrimitiveType_Unstack:
      return new Unstack(primitive);
    case schema::PrimitiveType_Unique:
      return new Unique(primitive);
    case schema::PrimitiveType_TopK:
      return new TopK(primitive);
    case schema::PrimitiveType_MatMul:
      return new MatMul(primitive);
    case schema::PrimitiveType_QuantDTypeCast:
      return new QuantDTypeCast(primitive);
    case schema::PrimitiveType_EmbeddingLookup:
      return new EmbeddingLookup(primitive);
    case schema::PrimitiveType_Elu:
      return new Elu(primitive);
    case schema::PrimitiveType_DeDepthwiseConv2D:
      return new DeDepthwiseConv2D(primitive);
    case schema::PrimitiveType_Shape:
      return new Shape(primitive);
    case schema::PrimitiveType_Unsqueeze:
      return new Unsqueeze(primitive);
    case schema::PrimitiveType_BatchToSpace:
    case schema::PrimitiveType_BatchToSpaceND:
      return new BatchToSpace(primitive);
    case schema::PrimitiveType_SpaceToBatch:
      return new SpaceToBatch(primitive);
    case schema::PrimitiveType_SpaceToBatchND:
      return new SpaceToBatchND(primitive);
    case schema::PrimitiveType_BroadcastTo:
      return new BroadcastTo(primitive);
    case schema::PrimitiveType_DepthToSpace:
      return new DepthToSpace(primitive);
    case schema::PrimitiveType_Lstm:
      return new Lstm(primitive);
    case schema::PrimitiveType_ZerosLike:
      return new ZerosLike(primitive);
    case schema::PrimitiveType_MakeTuple:
      return new MakeTuple(primitive);
    case schema::PrimitiveType_Where:
      return new Where(primitive);
    case schema::PrimitiveType_ScatterND:
      return new ScatterND(primitive);
    case schema::PrimitiveType_ConstantOfShape:
      return new ConstantOfShape(primitive);
    case schema::PrimitiveType_L2Norm:
      return new L2Norm(primitive);
    case schema::PrimitiveType_SparseToDense:
      return new SparseToDense(primitive);
    case schema::PrimitiveType_DetectionPostProcess:
      return new DetectionPostProcess(primitive);
    case schema::PrimitiveType_Dropout:
      return new Dropout(primitive);
    case schema::PrimitiveType_Neg:
      return new Neg(primitive);

#ifdef SUPPORT_TRAIN
    case schema::PrimitiveType_ActivationGrad:
      return new ActivationGrad(primitive);
    case schema::PrimitiveType_PoolingGrad:
      return new PoolingGrad(primitive);
    case schema::PrimitiveType_Conv2DGradFilter:
      return new Conv2DGradFilter(primitive);
    case schema::PrimitiveType_Conv2DGradInput:
      return new Conv2DGradInput(primitive);
    case schema::PrimitiveType_BiasGrad:
      return new BiasGrad(primitive);
    case schema::PrimitiveType_ApplyMomentum:
      return new ApplyMomentum(primitive);
    case schema::PrimitiveType_BNGrad:
      return new BNGrad(primitive);
    case schema::PrimitiveType_AddGrad:
      return new ArithmeticGrad(primitive);
    case schema::PrimitiveType_SubGrad:
      return new ArithmeticGrad(primitive);
    case schema::PrimitiveType_MulGrad:
      return new ArithmeticGrad(primitive);
    case schema::PrimitiveType_DivGrad:
      return new ArithmeticGrad(primitive);
    case schema::PrimitiveType_SoftmaxCrossEntropy:
      return new SoftmaxCrossEntropy(primitive);
    case schema::PrimitiveType_PowerGrad:
      return new PowerGrad(primitive);
    case schema::PrimitiveType_Depend:
      return new Depend(primitive);
    case schema::PrimitiveType_FlattenGrad:
      return new FlattenGrad(primitive);
    case schema::PrimitiveType_NegGrad:
      return new NegGrad(primitive);
    case schema::PrimitiveType_LogGrad:
      return new LogGrad(primitive);
#endif

    default:
      MS_LOG(ERROR) << "Unsupported primitive type in Create : " << schema::EnumNamePrimitiveType(op_type);
      break;
  }
  return nullptr;
}
#else
PrimitiveC *PrimitiveC::Create(const schema::Primitive *primitive) {
  MS_ASSERT(primitive);
  auto op_type = primitive->value_type();
  switch (op_type) {
    case schema::PrimitiveType_SoftMax:
      return NewPrimitiveC<SoftMax>(primitive);
    case schema::PrimitiveType_Activation:
      return NewPrimitiveC<Activation>(primitive);
    case schema::PrimitiveType_Conv2D:
      return NewPrimitiveC<Conv2D>(primitive);
    case schema::PrimitiveType_DeConv2D:
      return NewPrimitiveC<DeConv2D>(primitive);
    case schema::PrimitiveType_Reduce:
      return NewPrimitiveC<Reduce>(primitive);
    case schema::PrimitiveType_Pooling:
      return NewPrimitiveC<Pooling>(primitive);
    case schema::PrimitiveType_ROIPooling:
      return NewPrimitiveC<ROIPooling>(primitive);
    case schema::PrimitiveType_DepthwiseConv2D:
      return NewPrimitiveC<DepthwiseConv2D>(primitive);
    case schema::PrimitiveType_FusedBatchNorm:
      return NewPrimitiveC<FusedBatchNorm>(primitive);
    case schema::PrimitiveType_BatchNorm:
      return NewPrimitiveC<BatchNorm>(primitive);
    case schema::PrimitiveType_FullConnection:
      return NewPrimitiveC<FullConnection>(primitive);
    case schema::PrimitiveType_Power:
      return NewPrimitiveC<Power>(primitive);
    case schema::PrimitiveType_Pad:
      return NewPrimitiveC<Pad>(primitive);
    case schema::PrimitiveType_Range:
      return NewPrimitiveC<Range>(primitive);
    case schema::PrimitiveType_Mul:
      return NewPrimitiveC<Mul>(primitive);
    case schema::PrimitiveType_Add:
      return NewPrimitiveC<Add>(primitive);
    case schema::PrimitiveType_Sub:
      return NewPrimitiveC<Sub>(primitive);
    case schema::PrimitiveType_Div:
      return NewPrimitiveC<Div>(primitive);
    case schema::PrimitiveType_BiasAdd:
      return NewPrimitiveC<BiasAdd>(primitive);
    case schema::PrimitiveType_ExpandDims:
      return NewPrimitiveC<ExpandDims>(primitive);
    case schema::PrimitiveType_ArgMax:
      return NewPrimitiveC<ArgMax>(primitive);
    case schema::PrimitiveType_ArgMin:
      return NewPrimitiveC<ArgMin>(primitive);
    case schema::PrimitiveType_Cast:
      return NewPrimitiveC<Cast>(primitive);
    case schema::PrimitiveType_Reshape:
      return NewPrimitiveC<Reshape>(primitive);
    case schema::PrimitiveType_Scale:
      return NewPrimitiveC<Scale>(primitive);
    case schema::PrimitiveType_Eltwise:
      return NewPrimitiveC<Eltwise>(primitive);
    case schema::PrimitiveType_Ceil:
      return NewPrimitiveC<Ceil>(primitive);
    case schema::PrimitiveType_Concat:
      return NewPrimitiveC<Concat>(primitive);
    case schema::PrimitiveType_Fill:
      return NewPrimitiveC<Fill>(primitive);
    case schema::PrimitiveType_Nhwc2Nchw:
      return NewPrimitiveC<Nhwc2Nchw>(primitive);
    case schema::PrimitiveType_Nchw2Nhwc:
      return NewPrimitiveC<Nchw2Nhwc>(primitive);
    case schema::PrimitiveType_Transpose:
      return NewPrimitiveC<Transpose>(primitive);
    case schema::PrimitiveType_Slice:
      return NewPrimitiveC<Slice>(primitive);
    case schema::PrimitiveType_Squeeze:
      return NewPrimitiveC<Squeeze>(primitive);
    case schema::PrimitiveType_Flatten:
      return NewPrimitiveC<Flatten>(primitive);
    case schema::PrimitiveType_Mean:
      return NewPrimitiveC<Mean>(primitive);
    case schema::PrimitiveType_Stack:
      return NewPrimitiveC<Stack>(primitive);
    case schema::PrimitiveType_Crop:
      return NewPrimitiveC<Crop>(primitive);
    case schema::PrimitiveType_SquaredDifference:
      return NewPrimitiveC<SquaredDifference>(primitive);
    case schema::PrimitiveType_AddN:
      return NewPrimitiveC<AddN>(primitive);
    case schema::PrimitiveType_Abs:
      return NewPrimitiveC<Abs>(primitive);
    case schema::PrimitiveType_Sin:
      return NewPrimitiveC<Sin>(primitive);
    case schema::PrimitiveType_Cos:
      return NewPrimitiveC<Cos>(primitive);
    case schema::PrimitiveType_Log:
      return NewPrimitiveC<Log>(primitive);
    case schema::PrimitiveType_Neg:
      return NewPrimitiveC<Neg>(primitive);
    case schema::PrimitiveType_Sqrt:
      return NewPrimitiveC<Sqrt>(primitive);
    case schema::PrimitiveType_Rsqrt:
      return NewPrimitiveC<Rsqrt>(primitive);
    case schema::PrimitiveType_Square:
      return NewPrimitiveC<Square>(primitive);
    case schema::PrimitiveType_Exp:
      return NewPrimitiveC<Exp>(primitive);
    case schema::PrimitiveType_Gather:
      return NewPrimitiveC<Gather>(primitive);
    case schema::PrimitiveType_GatherNd:
      return NewPrimitiveC<GatherNd>(primitive);
    case schema::PrimitiveType_LocalResponseNormalization:
      return NewPrimitiveC<LocalResponseNormalization>(primitive);
    case schema::PrimitiveType_Maximum:
      return NewPrimitiveC<Maximum>(primitive);
    case schema::PrimitiveType_Minimum:
      return NewPrimitiveC<Minimum>(primitive);
    case schema::PrimitiveType_StridedSlice:
      return NewPrimitiveC<StridedSlice>(primitive);
    case schema::PrimitiveType_LeakyReLU:
      return NewPrimitiveC<LeakyReLU>(primitive);
    case schema::PrimitiveType_PReLU:
      return NewPrimitiveC<PReLU>(primitive);
    case schema::PrimitiveType_Round:
      return NewPrimitiveC<Round>(primitive);
    case schema::PrimitiveType_Reverse:
      return NewPrimitiveC<Reverse>(primitive);
    case schema::PrimitiveType_ReverseSequence:
      return NewPrimitiveC<ReverseSequence>(primitive);
    case schema::PrimitiveType_LogicalAnd:
      return NewPrimitiveC<LogicalAnd>(primitive);
    case schema::PrimitiveType_LogicalOr:
      return NewPrimitiveC<LogicalOr>(primitive);
    case schema::PrimitiveType_LogicalNot:
      return NewPrimitiveC<LogicalNot>(primitive);
    case schema::PrimitiveType_FloorDiv:
      return NewPrimitiveC<FloorDiv>(primitive);
    case schema::PrimitiveType_FloorMod:
      return NewPrimitiveC<FloorMod>(primitive);
    case schema::PrimitiveType_Equal:
      return NewPrimitiveC<Equal>(primitive);
    case schema::PrimitiveType_NotEqual:
      return NewPrimitiveC<NotEqual>(primitive);
    case schema::PrimitiveType_Less:
      return NewPrimitiveC<Less>(primitive);
    case schema::PrimitiveType_LessEqual:
      return NewPrimitiveC<LessEqual>(primitive);
    case schema::PrimitiveType_Greater:
      return NewPrimitiveC<Greater>(primitive);
    case schema::PrimitiveType_GreaterEqual:
      return NewPrimitiveC<GreaterEqual>(primitive);
    case schema::PrimitiveType_Floor:
      return NewPrimitiveC<Floor>(primitive);
    case schema::PrimitiveType_Split:
      return NewPrimitiveC<Split>(primitive);
    case schema::PrimitiveType_OneHot:
      return NewPrimitiveC<OneHot>(primitive);
    case schema::PrimitiveType_PriorBox:
      return NewPrimitiveC<PriorBox>(primitive);
    case schema::PrimitiveType_SpaceToDepth:
      return NewPrimitiveC<SpaceToDepth>(primitive);
    case schema::PrimitiveType_Tile:
      return NewPrimitiveC<Tile>(primitive);
    case schema::PrimitiveType_Resize:
      return NewPrimitiveC<Resize>(primitive);
    case schema::PrimitiveType_Unstack:
      return NewPrimitiveC<Unstack>(primitive);
    case schema::PrimitiveType_Unique:
      return NewPrimitiveC<Unique>(primitive);
    case schema::PrimitiveType_TopK:
      return NewPrimitiveC<TopK>(primitive);
    case schema::PrimitiveType_MatMul:
      return NewPrimitiveC<MatMul>(primitive);
    case schema::PrimitiveType_QuantDTypeCast:
      return NewPrimitiveC<QuantDTypeCast>(primitive);
    case schema::PrimitiveType_EmbeddingLookup:
      return NewPrimitiveC<EmbeddingLookup>(primitive);
    case schema::PrimitiveType_Elu:
      return NewPrimitiveC<Elu>(primitive);
    case schema::PrimitiveType_DeDepthwiseConv2D:
      return NewPrimitiveC<DeDepthwiseConv2D>(primitive);
    case schema::PrimitiveType_Shape:
      return NewPrimitiveC<Shape>(primitive);
    case schema::PrimitiveType_Unsqueeze:
      return NewPrimitiveC<Unsqueeze>(primitive);
    case schema::PrimitiveType_BatchToSpace:
    case schema::PrimitiveType_BatchToSpaceND:
      return NewPrimitiveC<BatchToSpace>(primitive);
    case schema::PrimitiveType_SpaceToBatch:
      return NewPrimitiveC<SpaceToBatch>(primitive);
    case schema::PrimitiveType_SpaceToBatchND:
      return NewPrimitiveC<SpaceToBatchND>(primitive);
    case schema::PrimitiveType_BroadcastTo:
      return NewPrimitiveC<BroadcastTo>(primitive);
    case schema::PrimitiveType_DepthToSpace:
      return NewPrimitiveC<DepthToSpace>(primitive);
    case schema::PrimitiveType_Lstm:
      return NewPrimitiveC<Lstm>(primitive);
    case schema::PrimitiveType_ZerosLike:
      return NewPrimitiveC<ZerosLike>(primitive);
    case schema::PrimitiveType_MakeTuple:
      return NewPrimitiveC<MakeTuple>(primitive);
    case schema::PrimitiveType_Where:
      return NewPrimitiveC<Where>(primitive);
    case schema::PrimitiveType_ScatterND:
      return NewPrimitiveC<ScatterND>(primitive);
    case schema::PrimitiveType_ConstantOfShape:
      return NewPrimitiveC<ConstantOfShape>(primitive);
    case schema::PrimitiveType_L2Norm:
      return NewPrimitiveC<L2Norm>(primitive);
    case schema::PrimitiveType_SparseToDense:
      return NewPrimitiveC<SparseToDense>(primitive);
    case schema::PrimitiveType_DetectionPostProcess:
      return NewPrimitiveC<DetectionPostProcess>(primitive);
    case schema::PrimitiveType_Dropout:
      return NewPrimitiveC<Dropout>(primitive);

#ifdef SUPPORT_TRAIN
    case schema::PrimitiveType_ActivationGrad:
      return NewPrimitiveC<ActivationGrad>(primitive);
    case schema::PrimitiveType_PoolingGrad:
      return NewPrimitiveC<PoolingGrad>(primitive);
    case schema::PrimitiveType_Conv2DGradFilter:
      return NewPrimitiveC<Conv2DGradFilter>(primitive);
    case schema::PrimitiveType_Conv2DGradInput:
      return NewPrimitiveC<Conv2DGradInput>(primitive);
    case schema::PrimitiveType_BiasGrad:
      return NewPrimitiveC<BiasGrad>(primitive);
    case schema::PrimitiveType_ApplyMomentum:
      return NewPrimitiveC<ApplyMomentum>(primitive);
    case schema::PrimitiveType_BNGrad:
      return NewPrimitiveC<BNGrad>(primitive);
    case schema::PrimitiveType_AddGrad:
      return NewPrimitiveC<ArithmeticGrad>(primitive);
    case schema::PrimitiveType_SubGrad:
      return NewPrimitiveC<ArithmeticGrad>(primitive);
    case schema::PrimitiveType_MulGrad:
      return NewPrimitiveC<ArithmeticGrad>(primitive);
    case schema::PrimitiveType_DivGrad:
      return NewPrimitiveC<ArithmeticGrad>(primitive);
    case schema::PrimitiveType_SoftmaxCrossEntropy:
      return NewPrimitiveC<SoftmaxCrossEntropy>(primitive);
    case schema::PrimitiveType_NegGrad:
      return NewPrimitiveC<NegGrad>(primitive);
    case schema::PrimitiveType_LogGrad:
      return NewPrimitiveC<LogGrad>(primitive);
#endif
    default:
      MS_LOG(ERROR) << "Unsupported primitive type in Create : " << schema::EnumNamePrimitiveType(op_type);
      break;
  }
  return nullptr;
}
void PrimitiveC::SetQuantType(schema::QuantType quant_type) { this->quant_type_ = quant_type; }
schema::QuantType PrimitiveC::GetQuantType() const { return quant_type_; }
#endif

int PrimitiveC::Type() const {
  if (this->primitive_ == nullptr) {
    return schema::PrimitiveType_NONE;
  }
#ifdef PRIMITIVE_WRITEABLE
  return this->primitive_->value.type;
#else
  return this->primitive_->value_type();
#endif
}
bool PrimitiveC::GetInferFlag() const { return this->infer_flag_; }

void PrimitiveC::SetInferFlag(bool flag) { this->infer_flag_ = flag; }

int PrimitiveC::InferShape(std::vector<lite::Tensor *> inputs_, std::vector<lite::Tensor *> outputs_) {
  auto input = inputs_.front();
  MS_ASSERT(input != nullptr);
  auto output = outputs_.front();
  MS_ASSERT(output != nullptr);
  output->set_shape(input->shape());
  output->set_data_type(input->data_type());
  output->SetFormat(input->GetFormat());
  return 0;
}

}  // namespace lite
}  // namespace mindspore
