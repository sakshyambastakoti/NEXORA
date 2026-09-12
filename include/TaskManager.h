#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

constexpr size_t MAX_TASKS = 10;

struct NEXORATask {
    char id[16];
    char title[64];
    bool completed;
    uint8_t priority; // 1: Low, 2: Medium, 3: High
    uint32_t createdAt;
};

class TaskManager {
public:
    static TaskManager& instance();

    void begin();
    void update();

    bool addTask(const char* id, const char* title, uint8_t priority);
    bool deleteTask(const char* id);
    bool toggleTask(const char* id);
    bool setTaskStatus(const char* id, bool completed);
    void clearAllTasks();

    size_t getTaskCount() const { return _taskCount; }
    const NEXORATask* getTasks() const { return _tasks; }
    const NEXORATask* getTask(size_t index) const {
        if (index < _taskCount) return &_tasks[index];
        return nullptr;
    }

    String serializeTasksJson() const;
    bool deserializeTasksJson(const char* jsonStr);

private:
    TaskManager() = default;
    ~TaskManager() = default;
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    void saveToStorage();

    NEXORATask _tasks[MAX_TASKS];
    size_t _taskCount = 0;
};
