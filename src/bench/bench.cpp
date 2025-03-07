#include <benchmark/benchmark.h>

#include <base/trackinfo.h>
#include <base/threadpoolexecutor.h>
#include <base/spinlock.h>
#include <base/memory.h>
#include <base/rng.h>
#include <base/dataconverter.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/platform.h>
#include <base/uuid.h>
#include <base/shared_singleton.h>
#include <base/singleton.h>

#include <base/logger_impl.h>
#include <base/executor.h>
#include <base/rcu_ptr.h>
#include <base/uuidof.h>
#include <base/sfc64.h>
#include <base/mpmc_queue.h>
#include <stream/fft.h>

#include <player/api.h>

#ifdef XAMP_OS_WIN
#include <base/simd.h>
#else
#include <uuid/uuid.h>
#endif

#include <iostream>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <execution>
#include <unordered_set>

using namespace xamp::player;
using namespace xamp::base;
using namespace xamp::stream;

static void BM_RcuPtr(benchmark::State& state) {
    constexpr std::string_view kBM_RcuPtrLoggerName = "BM_RcuPtr";

    const auto thread_pool = MakeThreadPoolExecutor(kBM_RcuPtrLoggerName);
    XampLoggerFactory.GetLogger(kBM_RcuPtrLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    RcuPtr<int> total(std::make_shared<int>());

    for (auto _ : state) {
	    Executor::ParallelFor(*thread_pool, 0, length, [&total](auto item) {
			total.copy_update([item](auto cp) {
				*cp += item;
				});
        }, 128);
    }
}

static void BM_RcuPtrMutex(benchmark::State& state) {
    constexpr std::string_view kBM_RcuPtrMutexLoggerName = "BM_RcuPtrMutex";

    const auto thread_pool = MakeThreadPoolExecutor(kBM_RcuPtrMutexLoggerName);
    XampLoggerFactory.GetLogger(kBM_RcuPtrMutexLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    int total = 0;
    FastMutex mutex;

    for (auto _ : state) {
	    Executor::ParallelFor(*thread_pool, 0, length, [&total, &mutex](auto item) {
            std::lock_guard<FastMutex> guard{ mutex };
			total += item;
            }, 128);
    }
}

static void BM_BaseLineThreadPool(benchmark::State& state) {
    const auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        for (auto i = 0; i < length; ++i) {
            total += 1;
        }
    }
}

static void BM_RoundRobinPolicyThreadPool(benchmark::State& state) {
    constexpr std::string_view kBM_LeastLoadPolicyThreadPoolLoggerName = "BM_RoundRobinPolicyThreadPool";

    const auto thread_pool = MakeThreadPoolExecutor(
        kBM_LeastLoadPolicyThreadPoolLoggerName,
        TaskSchedulerPolicy::ROUND_ROBIN_POLICY);

    XampLoggerFactory.GetLogger(kBM_LeastLoadPolicyThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        Executor::ParallelFor(*thread_pool, 0, length, [&total](auto item) {
            total += item;
            }, 128);
    }
}

static void BM_RandomPolicyThreadPool(benchmark::State& state) {
    constexpr std::string_view kBM_RandomPolicyThreadPoolLoggerName = "BM_RandomPolicyThreadPool";

    const auto thread_pool = MakeThreadPoolExecutor(
        kBM_RandomPolicyThreadPoolLoggerName,
        TaskSchedulerPolicy::THREAD_LOCAL_RANDOM_POLICY);

    XampLoggerFactory.GetLogger(kBM_RandomPolicyThreadPoolLoggerName)
        ->SetLevel(LOG_LEVEL_OFF);

    const auto length = state.range(0);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        Executor::ParallelFor(*thread_pool, 0, length, [&total](auto item) {
            total += item;
            }, 128);
    }
}

static void BM_StdAsync(benchmark::State& state) {    
    auto std_async_parallel_for = [](auto& items, auto&& f, size_t batches = 4) {
        auto begin = std::begin(items);
        auto size = std::distance(begin, std::end(items));

        for (size_t i = 0; i < size; ++i) {
            std::vector<std::shared_future<void>> futures((std::min)(size - i, batches));
            for (auto& ff : futures) {
                ff = std::async(std::launch::async, [f, begin, i]() -> void {
                    f(*(begin + i));
                    });
                ++i;
            }
            for (auto& ff : futures) {
                ff.wait();
            }
        }
        };

    auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        std_async_parallel_for(n, [&total](auto item) {
            total += item;
            });
    }
}

#ifdef XAMP_OS_WIN
static void BM_StdForEachPar(benchmark::State& state) {
    auto length = state.range(0);
    std::vector<int> n(length);
    std::iota(n.begin(), n.end(), 1);
    std::atomic<int64_t> total;
    for (auto _ : state) {
        std::for_each(std::execution::par,
            n.begin(),
            n.end(),
            [&total](auto&& item)
            {
                total += item;
            });
    }
}
#endif

static void BM_default_random_engine(benchmark::State& state) {
    std::random_device rd;
    std::default_random_engine engine(rd());
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_Sfc64Random(benchmark::State& state) {
	Sfc64Engine<> engine;
    for (auto _ : state) {
        size_t n = std::uniform_int_distribution<int32_t>(INT32_MIN, INT32_MAX)(engine);
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_PRNG(benchmark::State& state) {
    PRNG prng;
    for (auto _ : state) {
        size_t n = prng.NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * sizeof(int64_t));
}

static void BM_PRNG_GetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = Singleton<PRNG>::GetInstance().NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_PRNG_SharedGetInstance(benchmark::State& state) {
    for (auto _ : state) {
        size_t n = SharedSingleton<PRNG>::GetInstance().NextInt64();
        benchmark::DoNotOptimize(n);
    }
    state.SetBytesProcessed(sizeof(int64_t) * static_cast<int64_t>(state.iterations()));
}

static void BM_ConvertToIntAvx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int32_t>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            Convert(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(length * static_cast<int64_t>(state.iterations()) * sizeof(float));
}

static void BM_ConvertToShortAvx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int16_t>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            Convert(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToInt2432Avx(benchmark::State& state) {
    auto length = state.range(0);

    auto output = Vector<int32_t>(length);
    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

#ifdef XAMP_USE_BENCHMAKR
static void BM_ConvertToInt2432(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int32_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertToInt2432Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToInt(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int32_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertInt32Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_ConvertToShort(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int16_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::
            ConvertInt16Bench(output.data(), input.data(), ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}
#endif

static void BM_InterleavedToPlanarConvertToInt8_AVX(benchmark::State& state) {
    auto length = state.range(0);

    const auto input = Singleton<PRNG>::GetInstance().NextBytes(length);
    auto left_ch = Vector<int8_t>(length);
    auto right_ch = Vector<int8_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto _ : state) {
        InterleaveToPlanar<int8_t, int8_t>::Convert(input.data(),
            left_ch.data(),
            right_ch.data(),
            length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt8(benchmark::State& state) {
    auto length = state.range(0);

    const auto input = Singleton<PRNG>::GetInstance().NextBytes(length);
    auto output = Vector<int8_t>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::INTERLEAVED,
            PackedFormat::PLANAR>::Convert(
                output.data(),
                input.data(),
                ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32_AVX(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);

    for (auto _ : state) {
        InterleaveToPlanar<float, int32_t>::Convert(input.data(),
            output.data(),
            output.data() + (length / 2),
            length);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_InterleavedToPlanarConvertToInt32(benchmark::State& state) {
    auto length = state.range(0);

    auto input = Singleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    auto output = Vector<int>(length);

    AudioFormat input_format;
    AudioFormat output_format;

    input_format.SetChannel(2);
    output_format.SetChannel(2);
    output_format.SetPackedFormat(PackedFormat::PLANAR);

    const auto ctx = MakeConvert(input_format, output_format, length / 2);

    for (auto _ : state) {
        DataConverter<PackedFormat::PLANAR,
            PackedFormat::INTERLEAVED>::Convert(
                output.data(),
                input.data(),
                ctx);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * length * sizeof(float));
}

static void BM_Find_RobinHoodHashSet(benchmark::State& state) {
    HashSet<int64_t> map;

    while (map.size() < 1000000) {
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64();
        if (!map.contains(value)) {
            map.emplace(value);
        }
    }
   
    for (auto _ : state) {
        auto val = SharedSingleton<PRNG>::GetInstance().NextInt64();
        state.PauseTiming();
        (void)map.find(val);
        state.ResumeTiming();
    }
}

static void BM_Find_unordered_set(benchmark::State& state) {
    std::unordered_set<int64_t> map;

    while (map.size() < 1000000) {
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64();
        if (!map.contains(value)) {
            map.emplace(value);
        }
    }

    for (auto _ : state) {
        auto val = SharedSingleton<PRNG>::GetInstance().NextInt64();
        state.PauseTiming();
        (void)map.find(val);
        state.ResumeTiming();
    }
}

static void BM_FowardListSort(benchmark::State& state) {
    auto length = state.range(0);
    ForwardList<TrackInfo> list;

    for (auto i = 0; i < length; ++i) {
        TrackInfo metadata;
        metadata.track = SharedSingleton<PRNG>::GetInstance().Next((std::numeric_limits<uint32_t>::max)());
        list.push_front(metadata);
    }

    for (auto _ : state) {
        list.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });
    }
}

static void BM_VectorSort(benchmark::State& state) {
    auto length = state.range(0);
    std::vector<TrackInfo> list;

    for (auto i = 0; i < length; ++i) {
        TrackInfo metadata;
        metadata.track = SharedSingleton<PRNG>::GetInstance().Next((std::numeric_limits<uint32_t>::max)());
        list.push_back(metadata);
    }

    for (auto _ : state) {
        std::ranges::stable_sort(list,
                                 [](const auto& first, const auto& last) {
	                                 return first.track < last.track;
                                 });
    }
}

static void BM_LruCache_GetOrAdd(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto i = 0; i < length; ++i) {
        cache.Add(
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000),\
            SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000));
    }
    
    for (auto _ : state) {
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.PauseTiming(); 
        cache.GetOrAdd(key, []() {
            return SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
            });
        state.ResumeTiming();        
    }
}

static void BM_LruCache_Add(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto _ : state) {
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.PauseTiming();                
        cache.Add(key, value);
        state.ResumeTiming();        
    }
}

static void BM_LruCache_AddOrUpdate(benchmark::State& state) {
    auto length = state.range(0);

    LruCache<int64_t, int64_t> cache(length / 2);

    for (auto _ : state) {
        auto key = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        auto value = SharedSingleton<PRNG>::GetInstance().NextInt64(1, 1000);
        state.PauseTiming();
        cache.AddOrUpdate(key, value);
        state.ResumeTiming();        
    }
}

static void BM_FFT(benchmark::State& state) {
    auto length = state.range(0);

    auto input = SharedSingleton<PRNG>::GetInstance().NextBytes<float>(length, -1.0, 1.0);
    FFT fft;
    fft.Init(length);
    for (auto _ : state) {
        auto& result = fft.Forward(input.data(), length);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_Builtin_UuidParse(benchmark::State& state) {
    const auto uuid_str = MakeUuidString();
    for (auto _ : state) {
        auto result = ParseUuidString(uuid_str);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

static void BM_UuidParse(benchmark::State& state) {
    const auto uuid_str = MakeUuidString();

    for (auto _ : state) {
        auto result = Uuid::FromString(uuid_str);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_UuidCompilerTime(benchmark::State& state) {
    using namespace UuidLiterals;

    for (auto _ : state) {
        auto result = "4293818B-D5E9-4FBE-BA14-B15A68F1DEEA"_uuid;
        benchmark::DoNotOptimize(result);
    }
}

static int32_t TestFunc(int32_t a, int32_t b) {
    return a + b;
}


static void BM_BindFrontTest_BaseLine(benchmark::State& state) {
    for (auto _ : state) {
        auto result = TestFunc(1, 2);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_BindFrontTest(benchmark::State& state) {
    auto f = bind_front(&TestFunc);
    for (auto _ : state) {
        int32_t result = std::invoke(f, 1, 2);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_MpmcQueue(benchmark::State& state) {
    static MpmcQueue<int>* queue;
    static std::vector<std::jthread> writer_thread;
    static std::atomic<bool> is_stop{ false };
    if (state.thread_index() == 0) {
        // Setup code here.
        queue = new MpmcQueue<int>(512 * state.threads());
        for (auto i = 0; i < 8; ++i) {
            writer_thread.emplace_back([&]() {
                while (!is_stop) {
                    queue->TryEnqueue(1);
                }
                });
        }
    }
    for (auto _ : state) {
        int element;
        queue->TryDequeue(element);
    }
    if (state.thread_index() == 0) {
        // Teardown code here.
        is_stop = true;
        writer_thread.clear();
        delete queue;
    }
}

static void BM_BlockingQueue(benchmark::State& state) {
    static BlockingQueue<int>* queue;
    static std::vector<std::jthread> writer_thread;
    static std::atomic<bool> is_stop{ false };
    if (state.thread_index() == 0) {
        // Setup code here.
        queue = new BlockingQueue<int>(512 * state.threads());
        for (auto i = 0; i < 8; ++i) {
            writer_thread.emplace_back([&]() {
                while (!is_stop) {
                    queue->TryEnqueue(1);
                }
                });
        }        
    }
    for (auto _ : state) {
        int element;
        queue->TryDequeue(element);
    }
    if (state.thread_index() == 0) {
        // Teardown code here.
        is_stop = true;
        writer_thread.clear();
        delete queue;
    }
}

static void BM_FastMutex(benchmark::State& state) {
    static FastMutex *m;
    if (state.thread_index() == 0) {
        // Setup code here.
        m = new FastMutex();
    }
    for (auto _ : state) {
        m->lock();
        m->unlock();
    }
    if (state.thread_index() == 0) {
        // Teardown code here.
        delete m;
    }
}

static void BM_Spinlock(benchmark::State& state) {
    static Spinlock *m;
    if (state.thread_index() == 0) {
        // Setup code here.
        m = new Spinlock();
    }
    for (auto _ : state) {
        m->lock();
        m->unlock();
    }
    if (state.thread_index() == 0) {
        // Teardown code here.
        delete m;
    }
}
//BENCHMARK(BM_MpmcQueue)->ThreadRange(4, 512);
//BENCHMARK(BM_BlockingQueue)->ThreadRange(4, 512);

//BENCHMARK(BM_StdAsync)->RangeMultiplier(2)->Range(8, 8 << 12);
//BENCHMARK(BM_StdForEachPar)->RangeMultiplier(2)->Range(8, 8 << 12);
BENCHMARK(BM_RandomPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 12);
BENCHMARK(BM_RoundRobinPolicyThreadPool)->RangeMultiplier(2)->Range(8, 8 << 12);
BENCHMARK(BM_BaseLineThreadPool)->RangeMultiplier(2)->Range(8, 8 << 12);

//BENCHMARK(BM_FastMutex)->ThreadRange(4, 32);
//BENCHMARK(BM_Spinlock)->ThreadRange(4, 32);

//BENCHMARK(BM_Builtin_UuidParse);
//BENCHMARK(BM_UuidParse);
//BENCHMARK(BM_UuidCompilerTime);

//BENCHMARK(BM_Sfc64Random);
//BENCHMARK(BM_default_random_engine);

//BENCHMARK(BM_PRNG);
//BENCHMARK(BM_PRNG_GetInstance);
//BENCHMARK(BM_PRNG_SharedGetInstance);

//BENCHMARK(BM_BindFrontTest);
//BENCHMARK(BM_BindFrontTest_BaseLine);

//BENCHMARK(BM_Find_unordered_set);
//BENCHMARK(BM_Find_RobinHoodHashSet);

//BENCHMARK(BM_FowardListSort)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_VectorSort)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_LruCache_GetOrAdd)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_Add)->RangeMultiplier(2)->Range(4096, 8 << 10);
//BENCHMARK(BM_LruCache_AddOrUpdate)->RangeMultiplier(2)->Range(4096, 8 << 10);

//BENCHMARK(BM_FastMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdMemset)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_FastMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);
//BENCHMARK(BM_StdtMemcpy)->RangeMultiplier(2)->Range(4096, 8 << 16);

//BENCHMARK(BM_ConvertToInt2432Avx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToInt2432)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToIntAvx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToInt)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToShortAvx)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_ConvertToShort)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32_AVX)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt8_AVX)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt8)->RangeMultiplier(2)->Range(4096, 8 << 12);
//BENCHMARK(BM_InterleavedToPlanarConvertToInt32)->RangeMultiplier(2)->Range(4096, 8 << 12);

//BENCHMARK(BM_FFT)->RangeMultiplier(2)->Range(4096, 8 << 12);

//#ifdef XAMP_OS_WIN
//BENCHMARK(BM_std_for_each_par)->RangeMultiplier(2)->Range(8, 8 << 8);
//#endif

//BENCHMARK(BM_RcuPtr)->RangeMultiplier(2)->Range(8, 8 << 8);
//BENCHMARK(BM_RcuPtrMutex)->RangeMultiplier(2)->Range(8, 8 << 8);

int main(int argc, char** argv) {
    XampLoggerFactory
        .AddDebugOutput()
        .Startup();

    // For debug use!
    XAMP_LOG_DEBUG("Logger init success.");

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return -1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
}
