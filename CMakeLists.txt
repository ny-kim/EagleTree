cmake_minimum_required(VERSION 3.5)
project(EagleTree)

message("Starting cmake...")

find_package(Boost 1.61.0 COMPONENTS serialization REQUIRED)
IF (Boost_FOUND)
    message(STATUS "Boost INCLUDE DIR: ${Boost_INCLUDE_DIR}")
    include_directories(${Boost_INCLUDE_DIR})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    BlockManagers/bm_gc_locality.cpp
    BlockManagers/bm_hot_cold_seperation.cpp
    BlockManagers/bm_k_modal_groups.cpp
    BlockManagers/bm_locality.cpp
    BlockManagers/bm_parent.cpp
    BlockManagers/bm_round_robin.cpp
    BlockManagers/bm_shortest_queue.cpp
    BlockManagers/bm_tags.cpp
    BlockManagers/Garbage_Collector_500.cpp
    BlockManagers/Garbage_Collector_Greedy.cpp
    BlockManagers/Garbage_Collector_LRU.cpp
    BlockManagers/k_modal_group.cpp
    BlockManagers/Migrator.cpp
    BlockManagers/wear_leveling_strategy.cpp
    Experiments/our_demo.cpp
    FTLs/DFTL.cpp
    FTLs/FAST.cpp
    FTLs/ftl_parent.cpp
    FTLs/page_ftl.cpp
    FTLs/page_ftl_in_flash.cpp
    MTRand/mtrand.cpp
    MTRand/mtreadme.txt
    OperatingSystem/external_sort.cpp
    OperatingSystem/File_Manager.cpp
    OperatingSystem/flexible_reader.cpp
    OperatingSystem/grace_hash_join.cpp
    OperatingSystem/operating_system.cpp
    OperatingSystem/OS_Schedulers.cpp
    OperatingSystem/thread_implementations.cpp
    Scheduler/events_queue.cpp
    Scheduler/scheduler.cpp
    Scheduler/Scheduling_Strategies.cpp
    Utilities/Free_Space_Meter.cpp
    Utilities/Individual_Threads_Statistics.cpp
    Utilities/Queue_Length_Statistics.cpp
    Utilities/random_order_iterator.cpp
    Utilities/state_visualiser.cpp
    Utilities/StatisticData.cpp
    Utilities/statistics_gatherer.cpp
    Utilities/Utilization_Meter.cpp
    Utilities/visual_tracer.cpp
    address.cpp
    block.cpp
    block_management.h
    bloom_filter.hpp
    config.cpp
    COPYING
    die.cpp
    event.cpp
    experiment_graphing.cpp
    experiment_result.cpp
    experiment_runner.cpp
    Operating_System.h
    package.cpp
    page.cpp
    page_hotness_measurer.cpp
    plane.cpp
    README
    scheduler.h
    sequential_pattern_detector.cpp
    ssd.conf
    ssd.conf.bak
    ssd.conf.testing
    ssd.cpp
    ssd.h
    Workload_Definitions.cpp)

add_executable(our_demo ${SOURCE_FILES})

if(Boost_FOUND)
    target_link_libraries(our_demo ${Boost_LIBRARIES})
endif()

