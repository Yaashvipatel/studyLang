
"use strict";

const express   = require("express");
const cors      = require("cors");
const { exec }  = require("child_process");
const fs        = require("fs");
const path      = require("path");

const app  = express();
const PORT = 3001;

app.use(cors());
app.use(express.json());

const PROJECT_ROOT = path.join(__dirname, "..");
app.use(express.static(PROJECT_ROOT));

app.get("/", (_req, res) =>
    res.sendFile(path.join(PROJECT_ROOT, "studylang_compiler.html"))
);

const SRC_PATH = path.join(__dirname, "../src");


function buildBinary(callback) {
    const base = `cd "${SRC_PATH}" && flex lexer.l && bison -d parser.y`;
    const compile_attempts = [
        `${base} && gcc -Wall -o studylang lex.yy.c parser.tab.c semantic.c icg.c main.c`,
        `${base} && gcc -Wall -o studylang lex.yy.c parser.tab.c semantic.c icg.c main.c -lfl`,
        `${base} && gcc -Wall -o studylang lex.yy.c parser.tab.c semantic.c icg.c main.c -ll`,
    ];

    function tryNext(i) {
        if (i >= compile_attempts.length) {
            callback(new Error(
                "Build failed. Make sure flex, bison, and gcc are installed.\n" +
                "  macOS : brew install flex bison gcc\n" +
                "  Ubuntu: sudo apt install flex bison gcc"
            ));
            return;
        }
        exec(compile_attempts[i], { timeout: 30000 }, (err) => {
            if (err) tryNext(i + 1);
            else     callback(null);
        });
    }
    tryNext(0);
}

function runBinary(inputs, callback) {
    const inputFile = path.join(SRC_PATH, "input.txt");
    fs.writeFileSync(inputFile, inputs.join("\n"));
    const cmd = `cd "${SRC_PATH}" && ./studylang -f "${inputFile}"`;
    exec(cmd, { timeout: 15000 }, (err, stdout, stderr) => {
        const combined = stdout + (stderr ? "\n" + stderr : "");
        if (err && !stdout) callback(new Error(combined || err.message));
        else callback(null, combined);
    });
}


function parseOutput(raw) {
    const result = {
        phase1: { text: "", errors: [], passed: false },
        phase2: { text: "", errors: [], passed: false },
        phase3: { text: "", errors: [], passed: false },
        phase4: { text: "", errors: [], passed: false },
        studyPlan: [],
        timetableHTML: "",
    };

    function extractBetween(start, end) {
        const s = raw.indexOf(start);
        if (s === -1) return "";
        const e = raw.indexOf(end, s + start.length);
        return e === -1 ? raw.slice(s + start.length) : raw.slice(s + start.length, e);
    }

    function extractErrorJSON(tag) {
        const marker = `@@${tag}_ERRORS@@`;
        const idx = raw.indexOf(marker);
        if (idx === -1) return [];
        const rest = raw.slice(idx + marker.length).trimStart();
        const end  = rest.indexOf('\n');
        const json = end === -1 ? rest : rest.slice(0, end);
        try { return JSON.parse(json) || []; }
        catch { return []; }
    }

    result.phase1.text   = extractBetween("@@PHASE1_START@@\n", "\n@@PHASE1_END@@").trim();
    result.phase2.text   = extractBetween("@@PHASE2_START@@\n", "\n@@PHASE2_END@@").trim();
    result.phase3.text   = extractBetween("@@PHASE3_START@@\n", "\n@@PHASE3_END@@").trim();
    result.phase4.text   = extractBetween("@@PHASE4_START@@\n", "\n@@PHASE4_END@@").trim();

    result.phase1.errors = extractErrorJSON("LEX");
    result.phase2.errors = extractErrorJSON("SYN");
    result.phase3.errors = extractErrorJSON("SEM");

    result.phase1.passed = !result.phase1.text.includes("[FAILED]") &&
                           !result.phase1.text.includes("[SKIPPED]") &&
                           result.phase1.errors.length === 0;
    result.phase2.passed = !result.phase2.text.includes("[FAILED]") &&
                           !result.phase2.text.includes("[SKIPPED]") &&
                           result.phase2.errors.length === 0;
    result.phase3.passed = !result.phase3.text.includes("[FAILED]") &&
                           !result.phase3.text.includes("[SKIPPED]") &&
                           result.phase3.errors.filter(e => !e.msg.startsWith("[WARNING]")).length === 0;
    result.phase4.passed = !result.phase4.text.includes("[FAILED]") &&
                           !result.phase4.text.includes("[SKIPPED]");

    const planLines = result.phase4.text.split("\n");
    for (const line of planLines) {
        const m = line.match(/\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|/);
        if (m) {
            result.studyPlan.push({
                num:      m[1].trim(),
                action:   m[2].trim(),
                subject:  m[3].trim(),
                duration: m[4].trim(),
                timeRef:  m[5].trim(),
                priority: m[6].trim(),
            });
        }
    }

    result.timetableHTML = buildTimetableHTML(result.studyPlan);
    return result;
}


function buildTimetableHTML(tasks) {
    if (!tasks.length) return "";
    return tasks.map(task => {
        const priClass = task.priority !== "-"
            ? `badge-priority-${task.priority.toLowerCase()}`
            : "badge-priority-normal";
        const badges = [
            task.duration !== "-"
                ? `<span class="badge badge-duration">⏱ ${task.duration}</span>` : "",
            task.timeRef  !== "-"
                ? `<span class="badge badge-time">📍 ${task.timeRef}</span>`     : "",
            task.priority !== "-"
                ? `<span class="badge ${priClass}">${task.priority.toUpperCase()}</span>` : "",
        ].filter(Boolean).join("");

        return `<div class="task-card">
  <div class="task-card-top">
    <div class="task-num">${task.num}</div>
    <div style="flex:1">
      <div class="task-action">${task.action}</div>
      <div class="task-subject">${task.subject}</div>
    </div>
    <button class="task-card-del" onclick="this.closest('.task-card').remove()">✕</button>
  </div>
  ${badges ? `<div class="task-badges">${badges}</div>` : ""}
</div>`;
    }).join("\n");
}


app.post("/compile", (req, res) => {
    const { inputs } = req.body;
    if (!inputs || !inputs.length)
        return res.status(400).json({ error: "No input provided." });

    buildBinary((buildErr) => {
        if (buildErr)
            return res.status(500).json({ error: buildErr.message });

        runBinary(inputs, (runErr, raw) => {
            if (runErr)
                return res.status(500).json({ error: runErr.message });

            const parsed = parseOutput(raw);
            res.json(parsed);
        });
    });
});

app.get("/health", (_req, res) => res.json({ status: "ok" }));


app.listen(PORT, () => {
    console.log(`\n  StudyLang Backend running on http://localhost:${PORT}\n`);
    buildBinary((err) => {
        if (err) console.warn("  [WARN] Initial build:", err.message);
        else     console.log("  [OK]  C binary compiled successfully.\n");
    });
});
