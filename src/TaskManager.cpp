#include "TaskManager.h"
#include "StorageManager.h"
#include "TimeManager.h"

TaskManager& TaskManager::instance() {
    static TaskManager inst;
    return inst;
}

void TaskManager::begin() {
    Serial.println(F("[TASKS] Initializing TaskManager..."));
    _taskCount = 0;
    String loaded = StorageManager::instance().loadTasks();
    if (loaded.length() > 5) {
        deserializeTasksJson(loaded.c_str());
    }

    // If zero tasks, create a few default welcome tasks
    if (_taskCount == 0) {
        addTask("t1", "Welcome to NEXORA Desk Station", 3);
        addTask("t2", "Connect Wi-Fi & MQTT Dashboard", 2);
        addTask("t3", "Check Weather & Alarms", 1);
    }
}

void TaskManager::update() {
    // Background updates if needed
}

bool TaskManager::addTask(const char* id, const char* title, uint8_t priority) {
    // Check if ID already exists
    for (size_t i = 0; i < _taskCount; i++) {
        if (strcmp(_tasks[i].id, id) == 0) {
            strncpy(_tasks[i].title, title, sizeof(_tasks[i].title));
            _tasks[i].priority = priority;
            saveToStorage();
            return true;
        }
    }

    if (_taskCount >= MAX_TASKS) {
        Serial.println(F("[TASKS] Task limit reached."));
        return false;
    }

    NEXORATask& t = _tasks[_taskCount++];
    strncpy(t.id, id, sizeof(t.id));
    strncpy(t.title, title, sizeof(t.title));
    t.completed = false;
    t.priority = priority;
    t.createdAt = (uint32_t)TimeManager::instance().getEpoch();

    saveToStorage();
    Serial.printf("[TASKS] Added task: %s - %s\n", id, title);
    return true;
}

bool TaskManager::deleteTask(const char* id) {
    for (size_t i = 0; i < _taskCount; i++) {
        if (strcmp(_tasks[i].id, id) == 0) {
            for (size_t j = i; j < _taskCount - 1; j++) {
                _tasks[j] = _tasks[j + 1];
            }
            _taskCount--;
            saveToStorage();
            Serial.printf("[TASKS] Deleted task: %s\n", id);
            return true;
        }
    }
    return false;
}

bool TaskManager::toggleTask(const char* id) {
    for (size_t i = 0; i < _taskCount; i++) {
        if (strcmp(_tasks[i].id, id) == 0) {
            _tasks[i].completed = !_tasks[i].completed;
            saveToStorage();
            return true;
        }
    }
    return false;
}

bool TaskManager::setTaskStatus(const char* id, bool completed) {
    for (size_t i = 0; i < _taskCount; i++) {
        if (strcmp(_tasks[i].id, id) == 0) {
            _tasks[i].completed = completed;
            saveToStorage();
            return true;
        }
    }
    return false;
}

void TaskManager::clearAllTasks() {
    _taskCount = 0;
    saveToStorage();
}

String TaskManager::serializeTasksJson() const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (size_t i = 0; i < _taskCount; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = _tasks[i].id;
        obj["title"] = _tasks[i].title;
        obj["completed"] = _tasks[i].completed;
        obj["priority"] = _tasks[i].priority;
        obj["created_at"] = _tasks[i].createdAt;
    }
    String output;
    serializeJson(doc, output);
    return output;
}

bool TaskManager::deserializeTasksJson(const char* jsonStr) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) return false;

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull()) return false;

    _taskCount = 0;
    for (JsonObject obj : arr) {
        if (_taskCount >= MAX_TASKS) break;
        NEXORATask& t = _tasks[_taskCount++];
        strncpy(t.id, obj["id"] | "id", sizeof(t.id));
        strncpy(t.title, obj["title"] | "Untitled", sizeof(t.title));
        t.completed = obj["completed"] | false;
        t.priority = obj["priority"] | 1;
        t.createdAt = obj["created_at"] | 0;
    }
    return true;
}

void TaskManager::saveToStorage() {
    StorageManager::instance().saveTasks(serializeTasksJson());
}
