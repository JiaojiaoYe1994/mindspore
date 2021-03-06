# Copyright 2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""
Testing cache operator with mappable datasets
"""
import os
import pytest
import mindspore.dataset as ds
import mindspore.dataset.vision.c_transforms as c_vision
from mindspore import log as logger
from util import save_and_check_md5

DATA_DIR = "../data/dataset/testImageNetData/train/"
COCO_DATA_DIR = "../data/dataset/testCOCO/train/"
COCO_ANNOTATION_FILE = "../data/dataset/testCOCO/annotations/train.json"
NO_IMAGE_DIR = "../data/dataset/testRandomData/"

GENERATE_GOLDEN = False


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic1():
    """
    Test mappable leaf with cache op right over the leaf

       Repeat
         |
     Map(decode)
         |
       Cache
         |
     ImageFolder
    """

    logger.info("Test cache map basic 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        session_id = 1

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(operations=decode_op, input_columns=["image"])
    ds1 = ds1.repeat(4)

    filename = "cache_map_01_result.npz"
    save_and_check_md5(ds1, filename, generate_golden=GENERATE_GOLDEN)

    logger.info("test_cache_map_basic1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic2():
    """
    Test mappable leaf with the cache op later in the tree above the map(decode)

       Repeat
         |
       Cache
         |
     Map(decode)
         |
     ImageFolder
    """

    logger.info("Test cache map basic 2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(operations=decode_op, input_columns=["image"], cache=some_cache)
    ds1 = ds1.repeat(4)

    filename = "cache_map_02_result.npz"
    save_and_check_md5(ds1, filename, generate_golden=GENERATE_GOLDEN)

    logger.info("test_cache_map_basic2 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic3():
    """
    Test a repeat under mappable cache

        Cache
          |
      Map(decode)
          |
        Repeat
          |
      ImageFolder
    """

    logger.info("Test cache basic 3")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.repeat(4)
    ds1 = ds1.map(operations=decode_op, input_columns=["image"], cache=some_cache)
    logger.info("ds1.dataset_size is ", ds1.get_dataset_size())

    num_iter = 0
    for _ in ds1.create_dict_iterator(num_epochs=1):
        logger.info("get data from dataset")
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info('test_cache_basic3 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic4():
    """
    Test different rows result in core dump
    """
    logger.info("Test cache basic 4")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")
    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.repeat(4)
    ds1 = ds1.map(operations=decode_op, input_columns=["image"])
    logger.info("ds1.dataset_size is ", ds1.get_dataset_size())
    shape = ds1.output_shapes()
    logger.info(shape)
    num_iter = 0
    for _ in ds1.create_dict_iterator(num_epochs=1):
        logger.info("get data from dataset")
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info('test_cache_basic4 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic5():
    """
    Test Map with non-deterministic TensorOps above cache

               repeat
                  |
             Map(decode, randomCrop)
                  |
                Cache
                  |
             ImageFolder

    """
    logger.info("Test cache failure 5")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    data = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    random_crop_op = c_vision.RandomCrop([512, 512], [200, 200, 200, 200])
    decode_op = c_vision.Decode()

    data = data.map(input_columns=["image"], operations=decode_op)
    data = data.map(input_columns=["image"], operations=random_crop_op)
    data = data.repeat(4)

    num_iter = 0
    for _ in data.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info('test_cache_failure5 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_basic6():
    """
    Test cache as root node

       cache
         |
      ImageFolder
    """
    logger.info("Test cache basic 6")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")
    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    num_iter = 0
    for _ in ds1.create_dict_iterator(num_epochs=1):
        logger.info("get data from dataset")
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 2
    logger.info('test_cache_basic6 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure1():
    """
    Test nested cache (failure)

        Repeat
          |
        Cache
          |
      Map(decode)
          |
        Cache
          |
      ImageFolder

    """
    logger.info("Test cache failure 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(operations=decode_op, input_columns=["image"], cache=some_cache)
    ds1 = ds1.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in ds1.create_dict_iterator(num_epochs=1):
            num_iter += 1
    assert "Nested cache operations is not supported!" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure1 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure2():
    """
    Test zip under cache (failure)

               repeat
                  |
                Cache
                  |
             Map(decode)
                  |
                 Zip
                |    |
      ImageFolder     ImageFolder

    """
    logger.info("Test cache failure 2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    ds2 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    dsz = ds.zip((ds1, ds2))
    decode_op = c_vision.Decode()
    dsz = dsz.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    dsz = dsz.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in dsz.create_dict_iterator():
            num_iter += 1
    assert "ZipOp is currently not supported as a descendant operator under a cache" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure2 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure3():
    """
    Test batch under cache (failure)

               repeat
                  |
                Cache
                  |
             Map(resize)
                  |
                Batch
                  |
             ImageFolder
    """
    logger.info("Test cache failure 3")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    ds1 = ds1.batch(2)
    resize_op = c_vision.Resize((224, 224))
    ds1 = ds1.map(input_columns=["image"], operations=resize_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in ds1.create_dict_iterator():
            num_iter += 1
    assert "Unexpected error. Expect positive row id: -1" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure3 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure4():
    """
    Test filter under cache (failure)

               repeat
                  |
                Cache
                  |
             Map(decode)
                  |
                Filter
                  |
             ImageFolder

    """
    logger.info("Test cache failure 4")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    ds1 = ds1.filter(predicate=lambda data: data < 11, input_columns=["label"])

    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in ds1.create_dict_iterator():
            num_iter += 1
    assert "FilterOp is currently not supported as a descendant operator under a cache" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure4 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure5():
    """
    Test Map with non-deterministic TensorOps under cache (failure)

               repeat
                  |
                Cache
                  |
             Map(decode, randomCrop)
                  |
             ImageFolder

    """
    logger.info("Test cache failure 5")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    data = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    random_crop_op = c_vision.RandomCrop([512, 512], [200, 200, 200, 200])
    decode_op = c_vision.Decode()

    data = data.map(input_columns=["image"], operations=decode_op)
    data = data.map(input_columns=["image"], operations=random_crop_op, cache=some_cache)
    data = data.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in data.create_dict_iterator():
            num_iter += 1
    assert "MapOp with non-deterministic TensorOps is currently not supported as a descendant of cache" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure5 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_failure6():
    """
    Test no-cache-supporting leaf ops with Map under cache (failure)

               repeat
                  |
                Cache
                  |
             Map(resize)
                  |
                Coco

    """
    logger.info("Test cache failure 6")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)
    data = ds.CocoDataset(COCO_DATA_DIR, annotation_file=COCO_ANNOTATION_FILE, task="Detection", decode=True)
    resize_op = c_vision.Resize((224, 224))

    data = data.map(input_columns=["image"], operations=resize_op, cache=some_cache)
    data = data.repeat(4)

    with pytest.raises(RuntimeError) as e:
        num_iter = 0
        for _ in data.create_dict_iterator():
            num_iter += 1
    assert "There is currently no support for CocoOp under cache" in str(e.value)

    assert num_iter == 0
    logger.info('test_cache_failure6 Ended.\n')


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_parameter_check():
    """
    Test illegal parameters for DatasetCache
    """

    logger.info("Test cache map parameter check")

    with pytest.raises(ValueError) as info:
        ds.DatasetCache(session_id=-1, size=0, spilling=True)
    assert "Input is not within the required interval" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id="1", size=0, spilling=True)
    assert "Argument session_id with value 1 is not of type (<class 'int'>,)" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=None, size=0, spilling=True)
    assert "Argument session_id with value None is not of type (<class 'int'>,)" in str(info.value)

    with pytest.raises(ValueError) as info:
        ds.DatasetCache(session_id=1, size=-1, spilling=True)
    assert "Input is not within the required interval" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=1, size="1", spilling=True)
    assert "Argument size with value 1 is not of type (<class 'int'>,)" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=1, size=None, spilling=True)
    assert "Argument size with value None is not of type (<class 'int'>,)" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=1, size=0, spilling="illegal")
    assert "Argument spilling with value illegal is not of type (<class 'bool'>,)" in str(info.value)

    with pytest.raises(RuntimeError) as err:
        ds.DatasetCache(session_id=1, size=0, spilling=True, hostname="illegal")
    assert "Unexpected error. now cache client has to be on the same host with cache server" in str(err.value)

    with pytest.raises(RuntimeError) as err:
        ds.DatasetCache(session_id=1, size=0, spilling=True, hostname="127.0.0.2")
    assert "Unexpected error. now cache client has to be on the same host with cache server" in str(err.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=1, size=0, spilling=True, port="illegal")
    assert "incompatible constructor arguments" in str(info.value)

    with pytest.raises(TypeError) as info:
        ds.DatasetCache(session_id=1, size=0, spilling=True, port="50052")
    assert "incompatible constructor arguments" in str(info.value)

    with pytest.raises(RuntimeError) as err:
        ds.DatasetCache(session_id=1, size=0, spilling=True, port=0)
    assert "Unexpected error. port must be positive" in str(err.value)

    with pytest.raises(RuntimeError) as err:
        ds.DatasetCache(session_id=1, size=0, spilling=True, port=65536)
    assert "Unexpected error. illegal port number" in str(err.value)

    with pytest.raises(TypeError) as err:
        ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=True)
    assert "Argument cache with value True is not of type" in str(err.value)

    logger.info("test_cache_map_parameter_check Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_running_twice1():
    """
    Executing the same pipeline for twice (from python), with cache injected after map

       Repeat
         |
       Cache
         |
     Map(decode)
         |
     ImageFolder
    """

    logger.info("Test cache map running twice 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1
    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1
    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8

    logger.info("test_cache_map_running_twice1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_running_twice2():
    """
    Executing the same pipeline for twice (from shell), with cache injected after leaf

       Repeat
         |
     Map(decode)
         |
       Cache
         |
     ImageFolder
    """

    logger.info("Test cache map running twice 2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_running_twice2 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_extra_small_size1():
    """
    Test running pipeline with cache of extra small size and spilling true

       Repeat
         |
     Map(decode)
         |
       Cache
         |
     ImageFolder
    """

    logger.info("Test cache map extra small size 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=1, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_extra_small_size1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_extra_small_size2():
    """
    Test running pipeline with cache of extra small size and spilling false

       Repeat
         |
       Cache
         |
     Map(decode)
         |
     ImageFolder
    """

    logger.info("Test cache map extra small size 2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=1, spilling=False)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_extra_small_size2 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_no_image():
    """
    Test cache with no dataset existing in the path

       Repeat
         |
     Map(decode)
         |
       Cache
         |
     ImageFolder
    """

    logger.info("Test cache map no image")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=1, spilling=False)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=NO_IMAGE_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    with pytest.raises(RuntimeError):
        num_iter = 0
        for _ in ds1.create_dict_iterator():
            num_iter += 1

    assert num_iter == 0
    logger.info("test_cache_map_no_image Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_parallel_pipeline1(shard):
    """
    Test running two parallel pipelines (sharing cache) with cache injected after leaf op

       Repeat
         |
     Map(decode)
         |
       Cache
         |
     ImageFolder
    """

    logger.info("Test cache map parallel pipeline 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, num_shards=2, shard_id=int(shard), cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 4
    logger.info("test_cache_map_parallel_pipeline1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_parallel_pipeline2(shard):
    """
    Test running two parallel pipelines (sharing cache) with cache injected after map op

       Repeat
         |
       Cache
         |
     Map(decode)
         |
     ImageFolder
    """

    logger.info("Test cache map parallel pipeline 2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, num_shards=2, shard_id=int(shard))
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 4
    logger.info("test_cache_map_parallel_pipeline2 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_parallel_workers():
    """
    Test cache with num_parallel_workers > 1 set for map op and leaf op

       Repeat
         |
       cache
         |
     Map(decode)
         |
      ImageFolder
    """

    logger.info("Test cache map parallel workers")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, num_parallel_workers=4)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, num_parallel_workers=4, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_parallel_workers Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_server_workers_1():
    """
    start cache server with --workers 1 and then test cache function

       Repeat
         |
       cache
         |
     Map(decode)
         |
      ImageFolder
    """

    logger.info("Test cache map server workers 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_server_workers_1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_server_workers_100():
    """
    start cache server with --workers 100 and then test cache function

       Repeat
         |
     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map server workers 100")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_server_workers_100 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_num_connections_1():
    """
    Test setting num_connections=1 in DatasetCache

       Repeat
         |
       cache
         |
     Map(decode)
         |
      ImageFolder
    """

    logger.info("Test cache map num_connections 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True, num_connections=1)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_num_connections_1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_num_connections_100():
    """
    Test setting num_connections=100 in DatasetCache

       Repeat
         |
     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map num_connections 100")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True, num_connections=100)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_num_connections_100 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_prefetch_size_1():
    """
    Test setting prefetch_size=1 in DatasetCache

       Repeat
         |
       cache
         |
     Map(decode)
         |
      ImageFolder
    """

    logger.info("Test cache map prefetch_size 1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True, prefetch_size=1)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_prefetch_size_1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_prefetch_size_100():
    """
    Test setting prefetch_size=100 in DatasetCache

       Repeat
         |
     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map prefetch_size 100")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True, prefetch_size=100)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(4)

    num_iter = 0
    for _ in ds1.create_dict_iterator():
        num_iter += 1

    logger.info("Number of data in ds1: {} ".format(num_iter))
    assert num_iter == 8
    logger.info("test_cache_map_prefetch_size_100 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_to_device():
    """
    Test cache with to_device

     DeviceQueue
         |
      EpochCtrl
         |
       Repeat
         |
     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map to_device")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)
    ds1 = ds1.repeat(4)
    ds1 = ds1.to_device()
    ds1.send()

    logger.info("test_cache_map_to_device Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_epoch_ctrl1():
    """
    Test using two-loops method to run several epochs

     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map epoch ctrl1")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)

    num_epoch = 5
    iter1 = ds1.create_dict_iterator(num_epochs=num_epoch)

    epoch_count = 0
    for _ in range(num_epoch):
        row_count = 0
        for _ in iter1:
            row_count += 1
        logger.info("Number of data in ds1: {} ".format(row_count))
        assert row_count == 2
        epoch_count += 1
    assert epoch_count == num_epoch
    logger.info("test_cache_map_epoch_ctrl1 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_epoch_ctrl2():
    """
    Test using two-loops method with infinite epochs

        cache
         |
     Map(decode)
         |
      ImageFolder
    """

    logger.info("Test cache map epoch ctrl2")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op, cache=some_cache)

    num_epoch = 5
    # iter1 will always assume there is a next epoch and never shutdown
    iter1 = ds1.create_dict_iterator()

    epoch_count = 0
    for _ in range(num_epoch):
        row_count = 0
        for _ in iter1:
            row_count += 1
        logger.info("Number of data in ds1: {} ".format(row_count))
        assert row_count == 2
        epoch_count += 1
    assert epoch_count == num_epoch

    # manually stop the iterator
    iter1.stop()
    logger.info("test_cache_map_epoch_ctrl2 Ended.\n")


@pytest.mark.skipif(os.environ.get('RUN_CACHE_TEST') != 'TRUE', reason="Require to bring up cache server")
def test_cache_map_epoch_ctrl3():
    """
    Test using two-loops method with infinite epochs over repeat

       repeat
         |
     Map(decode)
         |
       cache
         |
      ImageFolder
    """

    logger.info("Test cache map epoch ctrl3")
    if "SESSION_ID" in os.environ:
        session_id = int(os.environ['SESSION_ID'])
    else:
        raise RuntimeError("Testcase requires SESSION_ID environment variable")

    some_cache = ds.DatasetCache(session_id=session_id, size=0, spilling=True)

    # This DATA_DIR only has 2 images in it
    ds1 = ds.ImageFolderDataset(dataset_dir=DATA_DIR, cache=some_cache)
    decode_op = c_vision.Decode()
    ds1 = ds1.map(input_columns=["image"], operations=decode_op)
    ds1 = ds1.repeat(2)

    num_epoch = 5
    # iter1 will always assume there is a next epoch and never shutdown
    iter1 = ds1.create_dict_iterator()

    epoch_count = 0
    for _ in range(num_epoch):
        row_count = 0
        for _ in iter1:
            row_count += 1
        logger.info("Number of data in ds1: {} ".format(row_count))
        assert row_count == 4
        epoch_count += 1
    assert epoch_count == num_epoch

    # reply on garbage collector to destroy iter1

    logger.info("test_cache_map_epoch_ctrl3 Ended.\n")


if __name__ == '__main__':
    test_cache_map_basic1()
    test_cache_map_basic2()
    test_cache_map_basic3()
    test_cache_map_basic4()
    test_cache_map_failure1()
    test_cache_map_failure2()
    test_cache_map_failure3()
    test_cache_map_failure4()
