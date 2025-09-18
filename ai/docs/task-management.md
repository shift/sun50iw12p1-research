# Task Management Documentation

## Overview

This project uses a structured task management system to track atomic tasks for the HY300 projector porting project.

## Directory Structure

```
docs/tasks/
├── README.md              # This file
├── 001-task-name.md       # Active tasks (numbered sequentially)
├── 002-another-task.md
└── completed/             # Completed tasks archive
    ├── 001-setup-rom-analysis.md
    └── 002-extract-dram-params.md
```

## Task Lifecycle

### 1. Task Creation
- Create new task file with incrementing number
- Use format: `NNN-brief-description.md`
- Set status to "pending"
- Reference relevant context files in ai/contexts/

### 2. Task Execution
- Update status to "in_progress" when starting
- Make incremental commits as work progresses
- Update task file with progress notes

### 3. Task Completion
- Verify all acceptance criteria are met
- Update status to "completed"
- Move task file to docs/tasks/completed/
- Commit final changes

## Task Template

```markdown
# Task: [Brief Description]

**Status:** [pending|in_progress|completed]
**Priority:** [high|medium|low] 
**Assigned:** [AI Agent]
**Created:** [Date]
**Context:** [Reference to ai/contexts/ files]

## Description
[Detailed description of atomic task]

## Prerequisites
- [Dependencies and requirements]

## Acceptance Criteria
- [ ] [Specific, measurable criteria]

## Notes
[Additional information, discoveries, issues]
```

## Priority Guidelines

- **High:** Critical path items, blocking other tasks
- **Medium:** Important but not blocking
- **Low:** Nice-to-have, optimization tasks

## Git Integration

- Each task should result in focused, atomic commits
- Reference task number in commit messages
- Use descriptive commit messages explaining the "why"

## Next Task Counter

Current: 002